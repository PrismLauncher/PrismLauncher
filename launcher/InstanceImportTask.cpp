// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "InstanceImportTask.h"

#include "Application.h"
#include "FileSystem.h"

#include "QObjectPtr.h"
#include "archive/ArchiveReader.h"
#include "archive/ExtractZipTask.h"
#include "icons/IconList.h"
#include "icons/IconUtils.h"

#include "minecraft/MinecraftInstance.h"
#include "modplatform/flame/FlameInstanceCreationTask.h"
#include "modplatform/modrinth/ModrinthInstanceCreationTask.h"
#include "modplatform/technic/TechnicPackProcessor.h"

#include "settings/INISettingsObject.h"
#include "tasks/Task.h"

#include "net/ApiRequest.h"

#include <QFileInfo>
#include <QtConcurrentRun>
#include <memory>
#include <utility>

InstanceImportTask::InstanceImportTask(QUrl sourceUrl, bool trustedSource, QWidget* parent, QMap<QString, QString> extraInfo)
    : m_sourceUrl(std::move(sourceUrl)), m_trustedSource(trustedSource), m_extra_info(std::move(extraInfo)), m_parent(parent)
{}

bool InstanceImportTask::abort()
{
    if (!canAbort()) {
        return false;
    }

    bool wasAborted = false;
    if (m_task) {
        wasAborted = m_task->abort();
    }
    return wasAborted;
}

void InstanceImportTask::executeTask()
{
    setAbortable(true);

    if (m_sourceUrl.isLocalFile()) {
        m_archivePath = m_sourceUrl.toLocalFile();
        processZipPack();
    } else {
        setStatus(tr("Downloading modpack:\n%1").arg(m_sourceUrl.toString()));

        downloadFromUrl();
    }
}

void InstanceImportTask::downloadFromUrl()
{
    const QString path(m_sourceUrl.host() + '/' + m_sourceUrl.path());

    auto entry = APPLICATION->metacache()->resolveEntry("general", path);
    entry->setStale(true);
    m_archivePath = entry->getFullPath();

    auto filesNetJob = makeShared<NetJob>(tr("Modpack download"), APPLICATION->network());
    filesNetJob->addNetAction(Net::ApiRequest::makeCached(m_sourceUrl, entry));

    connect(filesNetJob.get(), &NetJob::succeeded, this, &InstanceImportTask::processZipPack);
    connect(filesNetJob.get(), &NetJob::progress, this, &InstanceImportTask::setProgress);
    connect(filesNetJob.get(), &NetJob::stepProgress, this, &InstanceImportTask::propagateStepProgress);
    connect(filesNetJob.get(), &NetJob::failed, this, &InstanceImportTask::emitFailed);
    connect(filesNetJob.get(), &NetJob::aborted, this, &InstanceImportTask::emitAborted);
    m_task.reset(filesNetJob);
    filesNetJob->start();
}
namespace {

QString cleanPath(const QString& path)
{
    if (path == ".") {
        return QString();
    }
    QString result = path;
    if (result.startsWith("./")) {
        result = result.mid(2);
    }
    return result;
}
}  // namespace

void InstanceImportTask::processZipPack()
{
    setStatus(tr("Attempting to determine instance type"));
    QDir extractDir(m_stagingPath);
    qDebug() << "Attempting to create instance from" << m_archivePath;

    // open the zip and find relevant files in it
    MMCZip::ArchiveReader packZip(m_archivePath);
    qDebug() << "Attempting to determine instance type";

    QString root;
    // NOTE: Prioritize modpack platforms that aren't searched for recursively.
    // Especially Flame has a very common filename for its manifest, which may appear inside overrides for example
    // https://docs.modrinth.com/docs/modpacks/format_definition/#storage
    auto detectInstance = [this, &extractDir, &root](MMCZip::ArchiveReader::File* f, bool& stop) {
        if (!isRunning()) {
            stop = true;
            return true;
        }
        auto fileName = f->filename();
        if (fileName == "modrinth.index.json") {
            // process as Modrinth pack
            qDebug() << "Modrinth:" << true;
            m_modpackType = ModpackType::Modrinth;
            stop = true;
        } else if (fileName == "bin/modpack.jar" || fileName == "bin/version.json") {
            // process as Technic pack
            qDebug() << "Technic:" << true;
            extractDir.mkpath("minecraft");
            extractDir.cd("minecraft");
            m_modpackType = ModpackType::Technic;
            stop = true;
        } else if (fileName == "manifest.json") {
            qDebug() << "Flame:" << true;
            m_modpackType = ModpackType::Flame;
            stop = true;
        } else if (QFileInfo fileInfo(fileName); fileInfo.fileName() == "instance.cfg") {
            qDebug() << "MultiMC:" << true;
            m_modpackType = ModpackType::MultiMC;
            root = cleanPath(fileInfo.path());
            stop = true;
        }
        QCoreApplication::processEvents();
        return true;
    };
    if (!packZip.parse(detectInstance)) {
        emitFailed(tr("Unable to open supplied modpack zip file."));
        return;
    }
    if (m_modpackType == ModpackType::Unknown) {
        emitFailed(tr("Archive does not contain a recognized modpack type."));
        return;
    }
    setStatus(tr("Extracting modpack"));

    // make sure we extract just the pack
    auto zipTask = makeShared<MMCZip::ExtractZipTask>(m_archivePath, extractDir, root);

    auto progressStep = std::make_shared<TaskStepProgress>();
    connect(zipTask.get(), &Task::finished, this, [this, progressStep] {
        progressStep->state = TaskStepState::Succeeded;
        stepProgress(*progressStep);
    });

    connect(zipTask.get(), &Task::succeeded, this, &InstanceImportTask::extractFinished, Qt::QueuedConnection);
    connect(zipTask.get(), &Task::aborted, this, &InstanceImportTask::emitAborted);
    connect(zipTask.get(), &Task::failed, this, [this, progressStep](QString reason) {
        progressStep->state = TaskStepState::Failed;
        stepProgress(*progressStep);
        emitFailed(std::move(reason));
    });
    connect(zipTask.get(), &Task::stepProgress, this, &InstanceImportTask::propagateStepProgress);

    connect(zipTask.get(), &Task::progress, this, [this, progressStep](qint64 current, qint64 total) {
        progressStep->update(current, total);
        stepProgress(*progressStep);
    });
    connect(zipTask.get(), &Task::status, this, [this, progressStep](QString status) {
        progressStep->status = std::move(status);
        stepProgress(*progressStep);
    });
    connect(zipTask.get(), &Task::warningLogged, this, [this](const QString& line) { m_Warnings.append(line); });
    m_task.reset(zipTask);
    zipTask->start();
}

void InstanceImportTask::extractFinished()
{
    setAbortable(false);
    QDir extractDir(m_stagingPath);

    qDebug() << "Fixing permissions for extracted pack files...";
    QDirIterator it(extractDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto filepath = it.next();
        QFileInfo file(filepath);
        auto permissions = QFile::permissions(filepath);
        auto origPermissions = permissions;
        if (file.isDir()) {
            // Folder +rwx for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser;
        } else {
            // File +rw for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;
        }
        if (origPermissions != permissions) {
            if (!QFile::setPermissions(filepath, permissions)) {
                logWarning(tr("Could not fix permissions for %1").arg(filepath));
            } else {
                qDebug() << "Fixed" << filepath;
            }
        }
    }

    switch (m_modpackType) {
        case ModpackType::MultiMC:
            processMultiMC();
            return;
        case ModpackType::Technic:
            processTechnic();
            return;
        case ModpackType::Flame:
            processFlame();
            return;
        case ModpackType::Modrinth:
            processModrinth();
            return;
        case ModpackType::Unknown:
            emitFailed(tr("Archive does not contain a recognized modpack type."));
            return;
    }
}

namespace {

bool installIcon(const QString& root, const QString& instIconKey)
{
    auto importIconPath = IconUtils::findBestIconIn(root, instIconKey);
    if (importIconPath.isNull() || !QFile::exists(importIconPath)) {
        importIconPath = IconUtils::findBestIconIn(root, "icon.png");
    }
    if (importIconPath.isNull() || !QFile::exists(importIconPath)) {
        importIconPath = IconUtils::findBestIconIn(FS::PathCombine(root, "overrides"), "icon.png");
    }
    if (!importIconPath.isNull() && QFile::exists(importIconPath)) {
        // import icon
        auto* iconList = APPLICATION->icons();
        if (iconList->iconFileExists(instIconKey)) {
            iconList->deleteIcon(instIconKey);
        }
        iconList->installIcon(importIconPath, instIconKey + "." + QFileInfo(importIconPath).suffix());
        return true;
    }
    return false;
}
}  // namespace

void InstanceImportTask::processFlame()
{
    shared_qobject_ptr<FlameCreationTask> instCreationTask = nullptr;
    if (!m_extra_info.isEmpty()) {
        auto packIdIt = m_extra_info.constFind("pack_id");
        Q_ASSERT(packIdIt != m_extra_info.constEnd());
        const auto& packId = packIdIt.value();

        auto packVersionIdIt = m_extra_info.constFind("pack_version_id");
        Q_ASSERT(packVersionIdIt != m_extra_info.constEnd());
        const auto& packVersionId = packVersionIdIt.value();

        QString originalInstanceId;
        auto originalInstanceIdIt = m_extra_info.constFind("original_instance_id");
        if (originalInstanceIdIt != m_extra_info.constEnd()) {
            originalInstanceId = originalInstanceIdIt.value();
        }

        instCreationTask = makeShared<FlameCreationTask>(m_stagingPath, m_trustedSource, m_globalSettings, m_parent, packId, packVersionId,
                                                         originalInstanceId);
    } else {
        // FIXME: Find a way to get IDs in directly imported ZIPs
        instCreationTask = makeShared<FlameCreationTask>(m_stagingPath, m_trustedSource, m_globalSettings, m_parent, QString(), QString());
    }

    instCreationTask->setName(modifiedName());
    instCreationTask->setOriginalName(originalName(), version());
    // if the icon was specified by user, use that. otherwise pull icon from the pack
    if (m_instIcon == "default") {
        auto iconKey = QString("Flame_%1_Icon").arg(name());

        if (installIcon(m_stagingPath, iconKey)) {
            m_instIcon = iconKey;
        }
    }
    instCreationTask->setIcon(m_instIcon);
    instCreationTask->setGroup(m_instGroup);
    instCreationTask->setConfirmUpdate(shouldConfirmUpdate());

    auto weak = instCreationTask.toWeakRef();
    connect(instCreationTask.get(), &Task::succeeded, this, [this, weak] {
        if (auto sp = weak.lock()) {
            setOverride(sp->shouldOverride(), sp->originalInstanceID());
        }
        emitSucceeded();
    });
    connect(instCreationTask.get(), &Task::failed, this, &InstanceImportTask::emitFailed);
    connect(instCreationTask.get(), &Task::progress, this, &InstanceImportTask::setProgress);
    connect(instCreationTask.get(), &Task::stepProgress, this, &InstanceImportTask::propagateStepProgress);
    connect(instCreationTask.get(), &Task::status, this, &InstanceImportTask::setStatus);
    connect(instCreationTask.get(), &Task::details, this, &InstanceImportTask::setDetails);

    connect(instCreationTask.get(), &Task::aborted, this, &InstanceImportTask::emitAborted);
    connect(instCreationTask.get(), &Task::abortStatusChanged, this, &Task::setAbortable);
    connect(instCreationTask.get(), &Task::abortButtonTextChanged, this, &Task::setAbortButtonText);

    connect(instCreationTask.get(), &Task::warningLogged, this, [this](const QString& line) { m_Warnings.append(line); });

    m_task.reset(instCreationTask);
    setAbortable(true);
    m_task->start();
}

void InstanceImportTask::processTechnic()
{
    shared_qobject_ptr<Technic::TechnicPackProcessor> packProcessor{ new Technic::TechnicPackProcessor };
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &InstanceImportTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &InstanceImportTask::emitFailed);
    packProcessor->run(m_globalSettings, name(), m_instIcon, m_stagingPath);
}

void InstanceImportTask::processMultiMC()
{
    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(configPath);

    MinecraftInstance instance(m_globalSettings, std::move(instanceSettings), m_stagingPath);

    // reset time played on import... because packs.
    instance.resetTimePlayed();

    // UUID is carried over on export, but this is a distinct instance, so give it its own
    instance.regenerateUuid();

    // set a new nice name
    instance.setName(name());

    // if the icon was specified by user, use that. otherwise pull icon from the pack
    if (m_instIcon != "default") {
        instance.setIconKey(m_instIcon);
    } else {
        m_instIcon = instance.iconKey();

        installIcon(instance.instanceRoot(), m_instIcon);
    }
    emitSucceeded();
}

void InstanceImportTask::processModrinth()
{
    shared_qobject_ptr<ModrinthCreationTask> instCreationTask = nullptr;
    if (!m_extra_info.isEmpty()) {
        auto packIdIt = m_extra_info.constFind("pack_id");
        Q_ASSERT(packIdIt != m_extra_info.constEnd());
        const auto& packId = packIdIt.value();

        QString packVersionId;
        auto packVersionIdIt = m_extra_info.constFind("pack_version_id");
        if (packVersionIdIt != m_extra_info.constEnd()) {
            packVersionId = packVersionIdIt.value();
        }

        QString originalInstanceId;
        auto originalInstanceIdIt = m_extra_info.constFind("original_instance_id");
        if (originalInstanceIdIt != m_extra_info.constEnd()) {
            originalInstanceId = originalInstanceIdIt.value();
        }

        instCreationTask = makeShared<ModrinthCreationTask>(m_stagingPath, m_trustedSource, m_globalSettings, m_parent, packId,
                                                            packVersionId, originalInstanceId);
    } else {
        QString packId;
        if (!m_sourceUrl.isEmpty()) {
            static const QRegularExpression s_regex(R"(data\/([^\/]*)\/versions)");
            packId = s_regex.match(m_sourceUrl.toString()).captured(1);
        }

        // FIXME: Find a way to get the ID in directly imported ZIPs
        instCreationTask = makeShared<ModrinthCreationTask>(m_stagingPath, m_trustedSource, m_globalSettings, m_parent, packId);
    }

    instCreationTask->setName(modifiedName());
    instCreationTask->setOriginalName(originalName(), version());
    // if the icon was specified by user, use that. otherwise pull icon from the pack
    if (m_instIcon == "default") {
        auto iconKey = QString("Modrinth_%1_Icon").arg(name());

        if (installIcon(m_stagingPath, iconKey)) {
            m_instIcon = iconKey;
        }
    }
    instCreationTask->setIcon(m_instIcon);
    instCreationTask->setGroup(m_instGroup);
    instCreationTask->setConfirmUpdate(shouldConfirmUpdate());

    auto weak = instCreationTask.toWeakRef();
    connect(instCreationTask.get(), &Task::succeeded, this, [this, weak] {
        if (auto sp = weak.lock()) {
            setOverride(sp->shouldOverride(), sp->originalInstanceID());
        }
        emitSucceeded();
    });
    connect(instCreationTask.get(), &Task::failed, this, &InstanceImportTask::emitFailed);
    connect(instCreationTask.get(), &Task::progress, this, &InstanceImportTask::setProgress);
    connect(instCreationTask.get(), &Task::stepProgress, this, &InstanceImportTask::propagateStepProgress);
    connect(instCreationTask.get(), &Task::status, this, &InstanceImportTask::setStatus);
    connect(instCreationTask.get(), &Task::details, this, &InstanceImportTask::setDetails);

    connect(instCreationTask.get(), &Task::aborted, this, &InstanceImportTask::emitAborted);
    connect(instCreationTask.get(), &Task::abortStatusChanged, this, &Task::setAbortable);
    connect(instCreationTask.get(), &Task::abortButtonTextChanged, this, &Task::setAbortButtonText);

    connect(instCreationTask.get(), &Task::warningLogged, this, [this](const QString& line) { m_Warnings.append(line); });

    m_task.reset(instCreationTask);
    setAbortable(true);
    m_task->start();
}
