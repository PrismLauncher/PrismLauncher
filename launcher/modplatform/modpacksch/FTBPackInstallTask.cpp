// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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
#include "net/ChecksumValidator.h"
#include "settings/INISettingsObject.h"

#include "BuildConfig.h"
#include "Application.h"

namespace ModpacksCH {

PackInstallTask::PackInstallTask(Modpack pack, QString version)
{
    m_pack = pack;
    m_version_name = version;
}

bool PackInstallTask::abort()
{
    if(abortable)
    {
        return jobPtr->abort();
    }
    return false;
}

void PackInstallTask::executeTask()
{
    // Find pack version
    bool found = false;
    VersionInfo version;

    for(auto vInfo : m_pack.versions) {
        if (vInfo.name == m_version_name) {
            found = true;
            version = vInfo;
            break;
        }
    }

    if(!found) {
        emitFailed(tr("Failed to find pack version %1").arg(m_version_name));
        return;
    }

    auto *netJob = new NetJob("ModpacksCH::VersionFetch", APPLICATION->network());
    auto searchUrl = QString(BuildConfig.MODPACKSCH_API_BASE_URL + "public/modpack/%1/%2").arg(m_pack.id).arg(version.id);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &PackInstallTask::onDownloadSucceeded);
    QObject::connect(netJob, &NetJob::failed, this, &PackInstallTask::onDownloadFailed);
}

void PackInstallTask::onDownloadSucceeded()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from FTB at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    auto obj = doc.object();

    ModpacksCH::Version version;
    try
    {
        ModpacksCH::loadVersion(version, obj);
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        return;
    }
    m_version = version;

    downloadPack();
}

void PackInstallTask::onDownloadFailed(QString reason)
{
    jobPtr.reset();
    emitFailed(reason);
}

void PackInstallTask::downloadPack()
{
    setStatus(tr("Downloading mods..."));

    jobPtr = new NetJob(tr("Mod download"), APPLICATION->network());
    for(auto file : m_version.files) {
        if(file.serverOnly) continue;

        QFileInfo fileName(file.name);
        auto cacheName = fileName.completeBaseName() + "-" + file.sha1 + "." + fileName.suffix();

        auto entry = APPLICATION->metacache()->resolveEntry("ModpacksCHPacks", cacheName);
        entry->setStale(true);

        auto relpath = FS::PathCombine("minecraft", file.path, file.name);
        auto path = FS::PathCombine(m_stagingPath, relpath);

        if (filesToCopy.contains(path)) {
            qWarning() << "Ignoring" << file.url << "as a file of that path is already downloading.";
            continue;
        }
        qDebug() << "Will download" << file.url << "to" << path;
        filesToCopy[path] = entry->getFullPath();

        auto dl = Net::Download::makeCached(file.url, entry);
        if (!file.sha1.isEmpty()) {
            auto rawSha1 = QByteArray::fromHex(file.sha1.toLatin1());
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
        }
        jobPtr->addNetAction(dl);
    }

    connect(jobPtr.get(), &NetJob::succeeded, this, [&]()
    {
        abortable = false;
        jobPtr.reset();
        install();
    });
    connect(jobPtr.get(), &NetJob::failed, [&](QString reason)
    {
        abortable = false;
        jobPtr.reset();
        emitFailed(reason);
    });
    connect(jobPtr.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        abortable = true;
        setProgress(current, total);
    });

    jobPtr->start();
}

void PackInstallTask::install()
{
    setStatus(tr("Copying modpack files"));

    for (auto iter = filesToCopy.begin(); iter != filesToCopy.end(); iter++) {
        auto &to = iter.key();
        auto &from = iter.value();
        FS::copy fileCopyOperation(from, to);
        if(!fileCopyOperation()) {
            qWarning() << "Failed to copy" << from << "to" << to;
            emitFailed(tr("Failed to copy files"));
            return;
        }
    }

    setStatus(tr("Installing modpack"));

    auto instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
    instanceSettings->suspendSave();

    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    for(auto target : m_version.targets) {
        if(target.type == "game" && target.name == "minecraft") {
            components->setComponentVersion("net.minecraft", target.version, true);
            break;
        }
    }

    for(auto target : m_version.targets) {
        if(target.type != "modloader") continue;

        if(target.name == "forge") {
            components->setComponentVersion("net.minecraftforge", target.version, true);
        }
        else if(target.name == "fabric") {
            components->setComponentVersion("net.fabricmc.fabric-loader", target.version, true);
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

    instance.setName(m_instName);
    instance.setIconKey(m_instIcon);
    instance.setManagedPack("modpacksch", QString::number(m_pack.id), m_pack.name, QString::number(m_version.id), m_version.name);
    instanceSettings->resumeSave();

    emitSucceeded();
}

}
