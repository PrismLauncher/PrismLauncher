// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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
#include "MMCZip.h"
#include "NullInstance.h"

#include "icons/IconList.h"
#include "icons/IconUtils.h"

#include "modplatform/technic/TechnicPackProcessor.h"
#include "modplatform/modrinth/ModrinthInstanceCreationTask.h"
#include "modplatform/flame/FlameInstanceCreationTask.h"

#include "settings/INISettingsObject.h"

#include <QtConcurrentRun>
#include <algorithm>

#include <quazip/quazipdir.h>

InstanceImportTask::InstanceImportTask(const QUrl sourceUrl, QWidget* parent)
{
    m_sourceUrl = sourceUrl;
    m_parent = parent;
}

bool InstanceImportTask::abort()
{
    if (!canAbort())
        return false;

    if (m_filesNetJob)
        m_filesNetJob->abort();
    m_extractFuture.cancel();

    return Task::abort();
}

void InstanceImportTask::executeTask()
{
    setAbortable(true);

    if (m_sourceUrl.isLocalFile()) {
        m_archivePath = m_sourceUrl.toLocalFile();
        processZipPack();
    } else {
        setStatus(tr("Downloading modpack:\n%1").arg(m_sourceUrl.toString()));
        m_downloadRequired = true;

        const QString path(m_sourceUrl.host() + '/' + m_sourceUrl.path());

        auto entry = APPLICATION->metacache()->resolveEntry("general", path);
        entry->setStale(true);
        m_archivePath = entry->getFullPath();

        m_filesNetJob = new NetJob(tr("Modpack download"), APPLICATION->network());
        m_filesNetJob->addNetAction(Net::Download::makeCached(m_sourceUrl, entry));

        connect(m_filesNetJob.get(), &NetJob::succeeded, this, &InstanceImportTask::downloadSucceeded);
        connect(m_filesNetJob.get(), &NetJob::progress, this, &InstanceImportTask::downloadProgressChanged);
        connect(m_filesNetJob.get(), &NetJob::failed, this, &InstanceImportTask::downloadFailed);
        connect(m_filesNetJob.get(), &NetJob::aborted, this, &InstanceImportTask::downloadAborted);

        m_filesNetJob->start();
    }
}

void InstanceImportTask::downloadSucceeded()
{
    processZipPack();
    m_filesNetJob.reset();
}

void InstanceImportTask::downloadFailed(QString reason)
{
    emitFailed(reason);
    m_filesNetJob.reset();
}

void InstanceImportTask::downloadProgressChanged(qint64 current, qint64 total)
{
    setProgress(current, total);
}

void InstanceImportTask::downloadAborted()
{
    emitAborted();
    m_filesNetJob.reset();
}

void InstanceImportTask::processZipPack()
{
    setStatus(tr("Extracting modpack"));
    QDir extractDir(m_stagingPath);
    qCDebug(LAUNCHER_LOG) << "Attempting to create instance from" << m_archivePath;

    // open the zip and find relevant files in it
    m_packZip.reset(new QuaZip(m_archivePath));
    if (!m_packZip->open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Unable to open supplied modpack zip file."));
        return;
    }

    QuaZipDir packZipDir(m_packZip.get());

    // https://docs.modrinth.com/docs/modpacks/format_definition/#storage
    bool modrinthFound = packZipDir.exists("/modrinth.index.json");
    bool technicFound = packZipDir.exists("/bin/modpack.jar") || packZipDir.exists("/bin/version.json");
    QString root;

    // NOTE: Prioritize modpack platforms that aren't searched for recursively.
    // Especially Flame has a very common filename for its manifest, which may appear inside overrides for example
    if(modrinthFound)
    {
        // process as Modrinth pack
        qCDebug(LAUNCHER_LOG) << "Modrinth:" << modrinthFound;
        m_modpackType = ModpackType::Modrinth;
    }
    else if (technicFound)
    {
        // process as Technic pack
        qCDebug(LAUNCHER_LOG) << "Technic:" << technicFound;
        extractDir.mkpath(".minecraft");
        extractDir.cd(".minecraft");
        m_modpackType = ModpackType::Technic;
    }
    else
    {
        QString mmcRoot = MMCZip::findFolderOfFileInZip(m_packZip.get(), "instance.cfg");
        QString flameRoot = MMCZip::findFolderOfFileInZip(m_packZip.get(), "manifest.json");

        if (!mmcRoot.isNull())
        {
            // process as MultiMC instance/pack
            qCDebug(LAUNCHER_LOG) << "MultiMC:" << mmcRoot;
            root = mmcRoot;
            m_modpackType = ModpackType::MultiMC;
        }
        else if(!flameRoot.isNull())
        {
            // process as Flame pack
            qCDebug(LAUNCHER_LOG) << "Flame:" << flameRoot;
            root = flameRoot;
            m_modpackType = ModpackType::Flame;
        }
    }
    if(m_modpackType == ModpackType::Unknown)
    {
        emitFailed(tr("Archive does not contain a recognized modpack type."));
        return;
    }

    // make sure we extract just the pack
    m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractSubDir, m_packZip.get(), root, extractDir.absolutePath());
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &InstanceImportTask::extractFinished);
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &InstanceImportTask::extractAborted);
    m_extractFutureWatcher.setFuture(m_extractFuture);
}

void InstanceImportTask::extractFinished()
{
    m_packZip.reset();
    if (!m_extractFuture.result())
    {
        emitFailed(tr("Failed to extract modpack"));
        return;
    }
    QDir extractDir(m_stagingPath);

    qCDebug(LAUNCHER_LOG) << "Fixing permissions for extracted pack files...";
    QDirIterator it(extractDir, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        auto filepath = it.next();
        QFileInfo file(filepath);
        auto permissions = QFile::permissions(filepath);
        auto origPermissions = permissions;
        if(file.isDir())
        {
            // Folder +rwx for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser;
        }
        else
        {
            // File +rw for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;
        }
        if(origPermissions != permissions)
        {
            if(!QFile::setPermissions(filepath, permissions))
            {
                logWarning(tr("Could not fix permissions for %1").arg(filepath));
            }
            else
            {
                qCDebug(LAUNCHER_LOG) << "Fixed" << filepath;
            }
        }
    }

    switch(m_modpackType)
    {
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

void InstanceImportTask::extractAborted()
{
    emitAborted();
}

void InstanceImportTask::processFlame()
{
    auto* inst_creation_task = new FlameCreationTask(m_stagingPath, m_globalSettings, m_parent);

    inst_creation_task->setName(*this);
    inst_creation_task->setIcon(m_instIcon);
    inst_creation_task->setGroup(m_instGroup);
    
    connect(inst_creation_task, &Task::succeeded, this, [this, inst_creation_task] {
        setOverride(inst_creation_task->shouldOverride());
        emitSucceeded();
    });
    connect(inst_creation_task, &Task::failed, this, &InstanceImportTask::emitFailed);
    connect(inst_creation_task, &Task::progress, this, &InstanceImportTask::setProgress);
    connect(inst_creation_task, &Task::status, this, &InstanceImportTask::setStatus);
    connect(inst_creation_task, &Task::finished, inst_creation_task, &InstanceCreationTask::deleteLater);

    connect(this, &Task::aborted, inst_creation_task, &InstanceCreationTask::abort);
    connect(inst_creation_task, &Task::aborted, this, &Task::abort);
    connect(inst_creation_task, &Task::abortStatusChanged, this, &Task::setAbortable);

    inst_creation_task->start();
}

void InstanceImportTask::processTechnic()
{
    shared_qobject_ptr<Technic::TechnicPackProcessor> packProcessor = new Technic::TechnicPackProcessor();
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &InstanceImportTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &InstanceImportTask::emitFailed);
    packProcessor->run(m_globalSettings, name(), m_instIcon, m_stagingPath);
}

void InstanceImportTask::processMultiMC()
{
    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);

    NullInstance instance(m_globalSettings, instanceSettings, m_stagingPath);

    // reset time played on import... because packs.
    instance.resetTimePlayed();

    // set a new nice name
    instance.setName(name());

    // if the icon was specified by user, use that. otherwise pull icon from the pack
    if (m_instIcon != "default") {
        instance.setIconKey(m_instIcon);
    } else {
        m_instIcon = instance.iconKey();

        auto importIconPath = IconUtils::findBestIconIn(instance.instanceRoot(), m_instIcon);
        if (!importIconPath.isNull() && QFile::exists(importIconPath)) {
            // import icon
            auto iconList = APPLICATION->icons();
            if (iconList->iconFileExists(m_instIcon)) {
                iconList->deleteIcon(m_instIcon);
            }
            iconList->installIcons({ importIconPath });
        }
    }
    emitSucceeded();
}

void InstanceImportTask::processModrinth()
{
    auto* inst_creation_task = new ModrinthCreationTask(m_stagingPath, m_globalSettings, m_parent, m_sourceUrl.toString());

    inst_creation_task->setName(*this);
    inst_creation_task->setIcon(m_instIcon);
    inst_creation_task->setGroup(m_instGroup);
    
    connect(inst_creation_task, &Task::succeeded, this, [this, inst_creation_task] {
        setOverride(inst_creation_task->shouldOverride());
        emitSucceeded();
    });
    connect(inst_creation_task, &Task::failed, this, &InstanceImportTask::emitFailed);
    connect(inst_creation_task, &Task::progress, this, &InstanceImportTask::setProgress);
    connect(inst_creation_task, &Task::status, this, &InstanceImportTask::setStatus);
    connect(inst_creation_task, &Task::finished, inst_creation_task, &InstanceCreationTask::deleteLater);

    connect(this, &Task::aborted, inst_creation_task, &InstanceCreationTask::abort);
    connect(inst_creation_task, &Task::aborted, this, &Task::abort);
    connect(inst_creation_task, &Task::abortStatusChanged, this, &Task::setAbortable);

    inst_creation_task->start();
}
