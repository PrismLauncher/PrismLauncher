// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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
#include "modplatform/flame/PackManifest.h"
#include "net/ChecksumValidator.h"
#include "settings/INISettingsObject.h"

#include "Application.h"
#include "BuildConfig.h"
#include "ui/dialogs/BlockedModsDialog.h"

namespace ModpacksCH {

PackInstallTask::PackInstallTask(Modpack pack, QString version, QWidget* parent)
    : m_pack(std::move(pack)), m_version_name(std::move(version)), m_parent(parent)
{}

bool PackInstallTask::abort()
{
    if (!canAbort())
        return false;

    bool aborted = true;

    if (m_net_job)
        aborted &= m_net_job->abort();
    if (m_mod_id_resolver_task)
        aborted &= m_mod_id_resolver_task->abort();

    return aborted ? InstanceTask::abort() : false;
}

void PackInstallTask::executeTask()
{
    setStatus(tr("Getting the manifest..."));
    setAbortable(false);

    // Find pack version
    auto version_it = std::find_if(m_pack.versions.constBegin(), m_pack.versions.constEnd(),
                                   [this](ModpacksCH::VersionInfo const& a) { return a.name == m_version_name; });

    if (version_it == m_pack.versions.constEnd()) {
        emitFailed(tr("Failed to find pack version %1").arg(m_version_name));
        return;
    }

    auto version = *version_it;

    auto* netJob = new NetJob("ModpacksCH::VersionFetch", APPLICATION->network());

    auto searchUrl = QString(BuildConfig.MODPACKSCH_API_BASE_URL + "public/modpack/%1/%2").arg(m_pack.id).arg(version.id);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &m_response));

    QObject::connect(netJob, &NetJob::succeeded, this, &PackInstallTask::onManifestDownloadSucceeded);
    QObject::connect(netJob, &NetJob::failed, this, &PackInstallTask::onManifestDownloadFailed);
    QObject::connect(netJob, &NetJob::aborted, this, &PackInstallTask::abort);
    QObject::connect(netJob, &NetJob::progress, this, &PackInstallTask::setProgress);

    m_net_job = netJob;

    setAbortable(true);
    netJob->start();
}

void PackInstallTask::onManifestDownloadSucceeded()
{
    m_net_job.reset();

    QJsonParseError parse_error{};
    QJsonDocument doc = QJsonDocument::fromJson(m_response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qCWarning(LAUNCHER_LOG) << "Error while parsing JSON response from ModpacksCH at " << parse_error.offset
                   << " reason: " << parse_error.errorString();
        qCWarning(LAUNCHER_LOG) << m_response;
        return;
    }

    ModpacksCH::Version version;
    try {
        auto obj = Json::requireObject(doc);
        ModpacksCH::loadVersion(version, obj);
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

    m_file_id_map.clear();

    Flame::Manifest manifest;
    int index = 0;

    for (auto const& file : m_version.files) {
        if (!file.serverOnly && file.url.isEmpty()) {
            if (file.curseforge.file_id <= 0) {
                emitFailed(tr("Invalid manifest: There's no information available to download the file '%1'!").arg(file.name));
                return;
            }

            Flame::File flame_file;
            flame_file.projectId = file.curseforge.project_id;
            flame_file.fileId = file.curseforge.file_id;
            flame_file.hash = file.sha1;

            manifest.files.insert(flame_file.fileId, flame_file);
            m_file_id_map.append(flame_file.fileId);
        } else {
            m_file_id_map.append(-1);
        }

        index++;
    }

    m_mod_id_resolver_task = new Flame::FileResolvingTask(APPLICATION->network(), manifest);

    connect(m_mod_id_resolver_task.get(), &Flame::FileResolvingTask::succeeded, this, &PackInstallTask::onResolveModsSucceeded);
    connect(m_mod_id_resolver_task.get(), &Flame::FileResolvingTask::failed, this, &PackInstallTask::onResolveModsFailed);
    connect(m_mod_id_resolver_task.get(), &Flame::FileResolvingTask::aborted, this, &PackInstallTask::abort);
    connect(m_mod_id_resolver_task.get(), &Flame::FileResolvingTask::progress, this, &PackInstallTask::setProgress);

    setAbortable(true);

    m_mod_id_resolver_task->start();
}

void PackInstallTask::onResolveModsSucceeded()
{
    QString text;
    QList<QUrl> urls;
    auto anyBlocked = false;

    Flame::Manifest results = m_mod_id_resolver_task->getResults();
    for (int index = 0; index < m_file_id_map.size(); index++) {
        auto const file_id = m_file_id_map.at(index);
        if (file_id < 0)
            continue;

        Flame::File results_file = results.files[file_id];
        VersionFile& local_file = m_version.files[index];

        // First check for blocked mods
        if (!results_file.resolved || results_file.url.isEmpty()) {
            QString type(local_file.type);

            type[0] = type[0].toUpper();
            text += QString("%1: %2 - <a href='%3'>%3</a><br/>").arg(type, local_file.name, results_file.websiteUrl);
            urls.append(QUrl(results_file.websiteUrl));
            anyBlocked = true;
        } else {
            local_file.url = results_file.url.toString();
        }
    }

    m_mod_id_resolver_task.reset();

    if (anyBlocked) {
        qCDebug(LAUNCHER_LOG) << "Blocked files found, displaying file list";

        auto message_dialog = new BlockedModsDialog(m_parent, tr("Blocked files found"),
                                                   tr("The following files are not available for download in third party launchers.<br/>"
                                                      "You will need to manually download them and add them to the instance."),
                                                   text,
                                                   urls);

        if (message_dialog->exec() == QDialog::Accepted)
            createInstance();
        else
            abort();
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
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);

    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
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
    instance.setManagedPack("modpacksch", QString::number(m_pack.id), m_pack.name, QString::number(m_version.id), m_version.name);

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

    auto* jobPtr = new NetJob(tr("Mod download"), APPLICATION->network());
    for (auto const& file : m_version.files) {
        if (file.serverOnly || file.url.isEmpty())
            continue;

        auto path = FS::PathCombine(m_stagingPath, ".minecraft", file.path, file.name);
        qCDebug(LAUNCHER_LOG) << "Will try to download" << file.url << "to" << path;

        QFileInfo file_info(file.name);

        auto dl = Net::Download::makeFile(file.url, path);
        if (!file.sha1.isEmpty()) {
            auto rawSha1 = QByteArray::fromHex(file.sha1.toLatin1());
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
        }

        jobPtr->addNetAction(dl);
    }

    connect(jobPtr, &NetJob::succeeded, this, &PackInstallTask::onModDownloadSucceeded);
    connect(jobPtr, &NetJob::failed, this, &PackInstallTask::onModDownloadFailed);
    connect(jobPtr, &NetJob::aborted, this, &PackInstallTask::abort);
    connect(jobPtr, &NetJob::progress, this, &PackInstallTask::setProgress);

    m_net_job = jobPtr;

    setAbortable(true);
    jobPtr->start();
}

void PackInstallTask::onModDownloadSucceeded()
{
    m_net_job.reset();
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

}  // namespace ModpacksCH
