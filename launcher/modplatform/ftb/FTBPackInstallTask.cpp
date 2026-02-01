// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *      Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 *      Copyright 2020-2021 Petr Mrazek <peterix@gmail.com>
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

#include "FTBPackInstallTask.h"

#include "FileSystem.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/flame/FileResolvingTask.h"
#include "modplatform/flame/PackManifest.h"
#include "net/ChecksumValidator.h"
#include "settings/INISettingsObject.h"

#include "Application.h"
#include "BuildConfig.h"
#include "ui/dialogs/BlockedModsDialog.h"

namespace FTB {

PackInstallTask::PackInstallTask(Modpack pack, QString version, QWidget* parent)
    : m_pack(std::move(pack)), m_versionName(std::move(version)), m_parent(parent)
{}

bool PackInstallTask::abort()
{
    if (!canAbort())
        return false;

    bool aborted = true;

    if (m_net_job)
        aborted &= m_net_job->abort();
    if (m_modIdResolverTask)
        aborted &= m_modIdResolverTask->abort();

    return aborted ? InstanceTask::abort() : false;
}

void PackInstallTask::executeTask()
{
    setStatus(tr("Getting the manifest..."));
    setAbortable(false);

    // Find pack version
    auto version_it = std::find_if(m_pack.versions.constBegin(), m_pack.versions.constEnd(),
                                   [this](FTB::VersionInfo const& a) { return a.name == m_versionName; });

    if (version_it == m_pack.versions.constEnd()) {
        emitFailed(tr("Failed to find pack version %1").arg(m_versionName));
        return;
    }

    auto version = *version_it;

    auto netJob = makeShared<NetJob>("FTB::VersionFetch", APPLICATION->network());

    auto searchUrl = QString(BuildConfig.FTB_API_BASE_URL + "/modpack/%1/%2").arg(m_pack.id).arg(version.id);
    m_response.reset(new QByteArray());
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), m_response.get()));

    QObject::connect(netJob.get(), &NetJob::succeeded, this, &PackInstallTask::onManifestDownloadSucceeded);
    QObject::connect(netJob.get(), &NetJob::failed, this, &PackInstallTask::onManifestDownloadFailed);
    QObject::connect(netJob.get(), &NetJob::aborted, this, &PackInstallTask::abort);
    QObject::connect(netJob.get(), &NetJob::progress, this, &PackInstallTask::setProgress);

    m_net_job = netJob;

    setAbortable(true);
    netJob->start();
}

void PackInstallTask::onManifestDownloadSucceeded()
{
    m_net_job.reset();

    QJsonParseError parse_error{};
    QJsonDocument doc = QJsonDocument::fromJson(*m_response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from FTB at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << *m_response;
        return;
    }

    FTB::Version version;
    try {
        auto obj = Json::requireObject(doc);
        FTB::loadVersion(version, obj);
    } catch (const JSONValidationError& e) {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        return;
    }

    m_version = version;

    resolveMods();
}

void PackInstallTask::resolveMods()
{
    setStatus(tr("Resolving mods..."));
    setAbortable(false);
    setProgress(0, 100);

    m_fileIds.clear();

    Flame::Manifest manifest;
    for (auto const& file : m_version.files) {
        if (!file.serverOnly && file.url.isEmpty()) {
            if (file.curseforge.file_id <= 0) {
                emitFailed(tr("Invalid manifest: There's no information available to download the file '%1'!").arg(file.name));
                return;
            }

            Flame::File flameFile;
            flameFile.projectId = file.curseforge.project_id;
            flameFile.fileId = file.curseforge.file_id;

            manifest.files.insert(flameFile.fileId, flameFile);
            m_fileIds.append(flameFile.fileId);
        } else {
            m_fileIds.append(-1);
        }
    }

    m_modIdResolverTask.reset(new Flame::FileResolvingTask(manifest));

    connect(m_modIdResolverTask.get(), &Flame::FileResolvingTask::succeeded, this, &PackInstallTask::onResolveModsSucceeded);
    connect(m_modIdResolverTask.get(), &Flame::FileResolvingTask::failed, this, &PackInstallTask::onResolveModsFailed);
    connect(m_modIdResolverTask.get(), &Flame::FileResolvingTask::aborted, this, &PackInstallTask::abort);
    connect(m_modIdResolverTask.get(), &Flame::FileResolvingTask::progress, this, &PackInstallTask::setProgress);

    setAbortable(true);

    m_modIdResolverTask->start();
}

void PackInstallTask::onResolveModsSucceeded()
{
    auto anyBlocked = false;

    Flame::Manifest results = m_modIdResolverTask->getResults();
    for (int index = 0; index < m_fileIds.size(); index++) {
        auto const file_id = m_fileIds.at(index);
        if (file_id < 0)
            continue;

        Flame::File resultsFile = results.files[file_id];
        VersionFile& localFile = m_version.files[index];

        // First check for blocked mods
        if (resultsFile.version.downloadUrl.isEmpty()) {
            BlockedMod blocked_mod;
            blocked_mod.name = resultsFile.version.fileName;
            blocked_mod.websiteUrl = QString("%1/download/%2").arg(resultsFile.pack.websiteUrl, QString::number(resultsFile.fileId));
            blocked_mod.hash = resultsFile.version.hash;
            blocked_mod.matched = false;
            blocked_mod.localPath = "";
            blocked_mod.targetFolder = resultsFile.targetFolder;

            m_blockedMods.append(blocked_mod);

            anyBlocked = true;
        } else {
            localFile.url = resultsFile.version.downloadUrl;
        }
    }

    m_modIdResolverTask.reset();

    if (anyBlocked) {
        qDebug() << "Blocked files found, displaying file list";

        BlockedModsDialog message_dialog(m_parent, tr("Blocked files found"),
                                         tr("The following files are not available for download in third party launchers.<br/>"
                                            "You will need to manually download them and add them to the instance."),
                                         m_blockedMods);

        message_dialog.setModal(true);

        if (message_dialog.exec() == QDialog::Accepted) {
            qDebug() << "Post dialog blocked mods list: " << m_blockedMods;
            createInstance();
        } else {
            abort();
        }

    } else {
        createInstance();
    }
}

void PackInstallTask::createInstance()
{
    setAbortable(false);

    setStatus(tr("Creating the instance..."));
    QCoreApplication::processEvents();

    auto instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(instanceConfigPath);

    MinecraftInstance instance(m_globalSettings, std::move(instanceSettings), m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    for (auto target : m_version.targets) {
        if (target.type == "game" && target.name == "minecraft") {
            components->setComponentVersion("net.minecraft", target.version, true);
            break;
        }
    }

    for (auto target : m_version.targets) {
        if (target.type != "modloader")
            continue;

        if (target.name == "forge") {
            components->setComponentVersion("net.minecraftforge", target.version);
        } else if (target.name == "fabric") {
            components->setComponentVersion("net.fabricmc.fabric-loader", target.version);
        } else if (target.name == "neoforge") {
            components->setComponentVersion("net.neoforged", target.version);
        } else if (target.name == "quilt") {
            components->setComponentVersion("org.quiltmc.quilt-loader", target.version);
        }
    }

    // install any jar mods
    QDir jarModsDir(FS::PathCombine(m_stagingPath, "minecraft", "jarmods"));
    if (jarModsDir.exists()) {
        QStringList jarMods;

        for (const auto& info : jarModsDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files)) {
            jarMods.push_back(info.absoluteFilePath());
        }

        components->installJarMods(jarMods);
    }

    components->saveNow();

    instance.setName(name());
    instance.setIconKey(m_instIcon);
    instance.setManagedPack("ftb", QString::number(m_pack.id), m_pack.name, QString::number(m_version.id), m_version.name);

    instance.saveNow();

    onCreateInstanceSucceeded();
}

void PackInstallTask::onCreateInstanceSucceeded()
{
    downloadPack();
}

void PackInstallTask::downloadPack()
{
    setStatus(tr("Downloading mods..."));
    setAbortable(false);

    auto jobPtr = makeShared<NetJob>(tr("Mod download"), APPLICATION->network());
    for (auto const& file : m_version.files) {
        if (file.serverOnly || file.url.isEmpty())
            continue;

        auto path = FS::PathCombine(m_stagingPath, ".minecraft", file.path, file.name);
        qDebug() << "Will try to download" << file.url << "to" << path;

        QFileInfo file_info(file.name);

        auto dl = Net::Download::makeFile(file.url, path);
        if (!file.sha1.isEmpty()) {
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, file.sha1));
        }

        jobPtr->addNetAction(dl);
    }

    connect(jobPtr.get(), &NetJob::succeeded, this, &PackInstallTask::onModDownloadSucceeded);
    connect(jobPtr.get(), &NetJob::failed, this, &PackInstallTask::onModDownloadFailed);
    connect(jobPtr.get(), &NetJob::aborted, this, &PackInstallTask::abort);
    connect(jobPtr.get(), &NetJob::progress, this, &PackInstallTask::setProgress);

    m_net_job = jobPtr;

    setAbortable(true);
    jobPtr->start();
}

void PackInstallTask::onModDownloadSucceeded()
{
    m_net_job.reset();
    if (!m_blockedMods.isEmpty()) {
        copyBlockedMods();
    }
    emitSucceeded();
}

void PackInstallTask::onManifestDownloadFailed(QString reason)
{
    m_net_job.reset();
    emitFailed(reason);
}
void PackInstallTask::onResolveModsFailed(QString reason)
{
    m_net_job.reset();
    emitFailed(reason);
}
void PackInstallTask::onCreateInstanceFailed(QString reason)
{
    emitFailed(reason);
}
void PackInstallTask::onModDownloadFailed(QString reason)
{
    m_net_job.reset();
    emitFailed(reason);
}

/// @brief copy the matched blocked mods to the instance staging area
void PackInstallTask::copyBlockedMods()
{
    setStatus(tr("Copying Blocked Mods..."));
    setAbortable(false);
    int i = 0;
    int total = m_blockedMods.length();
    setProgress(i, total);
    for (auto const& mod : m_blockedMods) {
        if (!mod.matched) {
            qDebug() << mod.name << "was not matched to a local file, skipping copy";
            continue;
        }

        auto dest_path = FS::PathCombine(m_stagingPath, ".minecraft", mod.targetFolder, mod.name);

        setStatus(tr("Copying Blocked Mods (%1 out of %2 are done)").arg(QString::number(i), QString::number(total)));

        qDebug() << "Will try to copy" << mod.localPath << "to" << dest_path;

        if (!FS::copy(mod.localPath, dest_path)()) {
            qDebug() << "Copy of" << mod.localPath << "to" << dest_path << "Failed";
        }

        i++;
        setProgress(i, total);
    }

    setAbortable(true);
}

}  // namespace FTB
