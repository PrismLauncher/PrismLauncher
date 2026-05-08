// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "FlameInstanceCreationTask.h"

#include "QObjectPtr.h"
#include "minecraft/mod/tasks/LocalResourceUpdateTask.h"
#include "modplatform/flame/FileResolvingTask.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/flame/PackManifest.h"

#include "Application.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "Json.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include "modplatform/helpers/OverrideUtils.h"

#include "settings/INISettingsObject.h"

#include "tasks/ConcurrentTask.h"
#include "ui/dialogs/BlockedModsDialog.h"
#include "ui/dialogs/CustomMessageBox.h"

#include <QDebug>
#include <QFileInfo>
#include <utility>

#include "HardwareInfo.h"
#include "meta/Index.h"
#include "minecraft/World.h"
#include "minecraft/mod/tasks/LocalResourceParse.h"
#include "net/ApiDownload.h"
#include "ui/pages/modplatform/OptionalModDialog.h"

bool FlameCreationTask::abort()
{
    if (!canAbort()) {
        return false;
    }

    if (m_processUpdateFileInfoJob) {
        m_processUpdateFileInfoJob->abort();
    }
    if (m_filesJob) {
        m_filesJob->abort();
    }
    if (m_modIdResolver) {
        m_modIdResolver->abort();
    }

    return InstanceTask::abort();
}

void FlameCreationTask::executeTask()
{
    auto* instanceList = APPLICATION->instances();

    // FIXME: How to handle situations when there's more than one install already for a given modpack?
    BaseInstance* inst = nullptr;
    if (auto originalId = originalInstanceID(); !originalId.isEmpty()) {
        inst = instanceList->getInstanceById(originalId);
        Q_ASSERT(inst);
    } else {
        inst = instanceList->getInstanceByManagedName(originalName());

        if (!inst) {
            inst = instanceList->getInstanceById(originalName());

            if (!inst) {
                createInstance();
                return;
            }
        }
    }

    const QString indexPath(FS::PathCombine(m_stagingPath, "manifest.json"));

    try {
        Flame::loadManifest(m_pack, indexPath);
    } catch (const JSONValidationError&) {
        // emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        createInstance();  // to keep the backwards comatibility here just create the instance
        return;
    }

    auto versionId = inst->getManagedPackVersionName();
    auto versionStr = !versionId.isEmpty() ? tr(" (version %1)").arg(versionId) : "";

    if (shouldConfirmUpdate()) {
        auto shouldUpdate = askIfShouldUpdate(m_parent, versionStr);
        if (shouldUpdate == ShouldUpdate::SkipUpdating) {
            createInstance();
            return;
        }
        if (shouldUpdate == ShouldUpdate::Cancel) {
            emitAborted();
            return;
        }
    }

    const QDir oldInstDir(inst->instanceRoot());

    const QString oldIndexFolder(FS::PathCombine(oldInstDir.absolutePath(), "flame"));
    const QString oldIndexPath(FS::PathCombine(oldIndexFolder, "manifest.json"));

    const QFileInfo oldIndexFile(oldIndexPath);
    auto createInst = [this, inst] {
        setOverride(true, inst->id());
        qDebug() << "Will override instance!";

        m_oldInstance = inst;

        // We let it go through the createInstance() stage, just with a couple modifications for updating
        createInstance();
    };

    auto warnUser = [this, createInst](const QString& title,
                                       const QString& text) {  // We don't have an old index file, so we may duplicate stuff!
        auto* dialog = CustomMessageBox::selectable(m_parent, title, text, QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Cancel);

        if (dialog->exec() == QDialog::DialogCode::Rejected) {
            emitAborted();
            return;
        }

        createInst();
    };

    if (oldIndexFile.exists()) {
        Flame::Manifest oldPack;
        Flame::loadManifest(oldPack, oldIndexPath);

        auto oldFiles = oldPack.files;

        auto& files = m_pack.files;

        // Remove repeated files, we don't need to download them!
        auto filesIterator = files.begin();
        while (filesIterator != files.end()) {
            const auto& file = filesIterator;

            auto oldFile = oldFiles.find(file.key());
            if (oldFile != oldFiles.end()) {
                // We found a match, but is it a different version?
                if (oldFile->fileId == file->fileId) {
                    qDebug() << "Removed file at" << file->targetFolder << "with id" << file->fileId << "from list of downloads";

                    oldFiles.remove(file.key());
                    filesIterator = files.erase(filesIterator);

                    if (filesIterator != files.begin()) {
                        filesIterator--;
                    }
                }
            }

            filesIterator++;
        }

        const QDir oldMinecraftDir(inst->gameRoot());

        // We will remove all the previous overrides, to prevent duplicate files!
        // TODO: Currently 'overrides' will always override the stuff on update. How do we preserve unchanged overrides?
        // FIXME: We may want to do something about disabled mods.
        auto oldOverrides = Override::readOverrides("overrides", oldIndexFolder);
        for (const auto& entry : oldOverrides) {
            scheduleToDelete(m_parent, oldMinecraftDir, entry);
        }

        // Remove remaining old files (we need to do an API request to know which ids are which files...)
        QStringList fileIds;

        for (auto& file : oldFiles) {
            fileIds.append(QString::number(file.fileId));
        }

        auto [job, rawResponse] = FlameAPI().getFiles(fileIds);

        connect(job.get(), &Task::succeeded, this,
                [this, rawResponse, fileIds, oldInstDir, oldFiles, oldMinecraftDir, createInst]() mutable {
                    // Parse the API response
                    QJsonParseError parseError{};
                    auto doc = QJsonDocument::fromJson(*rawResponse, &parseError);
                    if (parseError.error != QJsonParseError::NoError) {
                        qWarning() << "Error while parsing JSON response from Flame files task at" << parseError.offset
                                   << "reason:" << parseError.errorString();
                        qWarning() << *rawResponse;
                        return;
                    }

                    try {
                        QJsonArray entries;
                        if (fileIds.size() == 1) {
                            entries = { Json::requireObject(Json::requireObject(doc), "data") };
                        } else {
                            entries = Json::requireArray(Json::requireObject(doc), "data");
                        }

                        for (auto entry : entries) {
                            auto entryObj = Json::requireObject(entry);

                            Flame::File file;
                            // We don't care about blocked mods, we just need local data to delete the file
                            file.version = FlameMod::loadIndexedPackVersion(entryObj);
                            auto id = Json::requireInteger(entryObj, "id");
                            oldFiles.insert(id, file);
                        }
                    } catch (Json::JsonException& e) {
                        qCritical() << e.cause() << e.what();
                    }

                    // Delete the files
                    for (const auto& file : oldFiles) {
                        if (file.version.fileName.isEmpty() || file.targetFolder.isEmpty()) {
                            continue;
                        }

                        const QString relativePath(FS::PathCombine(file.targetFolder, file.version.fileName));
                        scheduleToDelete(m_parent, oldMinecraftDir, relativePath, true);
                    }

                    createInst();
                });
        connect(job.get(), &Task::aborted, this, [warnUser] {
            warnUser(tr("Failed to fetch the old files."),
                     tr("We couldn't fetch the old files because the task was aborted. This may cause "
                        "some of the files to be duplicated. Do you want to continue?"));
        });
        connect(job.get(), &Task::failed, this, [warnUser](const QString& reason) {
            warnUser(tr("Failed to fetch the old files."), tr("We couldn't fetch the old files because: %1. This may cause some of the "
                                                              "files to be duplicated. Do you want to continue?")
                                                               .arg(reason));
        });

        m_processUpdateFileInfoJob = job;
        job->start();
        return;
    }
    warnUser(tr("No index file."), tr("We couldn't find a suitable index file for the older version. This may cause some of the files to "
                                      "be duplicated. Do you want to continue?"));
}

QString FlameCreationTask::getVersionForLoader(const QString& uid,
                                               const QString& loaderType,
                                               const QString& loaderVersion,
                                               const QString& mcVersion)
{
    if (loaderVersion == "recommended") {
        auto vlist = APPLICATION->metadataIndex()->get(uid);
        if (!vlist) {
            emitFailed(tr("Failed to get local metadata index for %1").arg(uid));
            return {};
        }

        if (!vlist->isLoaded()) {
            QEventLoop loadVersionLoop;
            auto task = vlist->getLoadTask();
            connect(task.get(), &Task::finished, &loadVersionLoop, &QEventLoop::quit);
            if (!task->isRunning()) {
                task->start();
            }

            loadVersionLoop.exec();
        }

        for (const auto& version : vlist->versions()) {
            // first recommended build we find, we use.
            if (!version->isRecommended()) {
                continue;
            }
            auto reqs = version->requiredSet();

            // filter by minecraft version, if the loader depends on a certain version.
            // not all mod loaders depend on a given Minecraft version, so we won't do this
            // filtering for those loaders.
            if (loaderType == "forge" || loaderType == "neoforge") {
                auto iter = std::find_if(reqs.begin(), reqs.end(), [mcVersion](const Meta::Require& req) {
                    return req.uid == "net.minecraft" && req.equalsVersion == mcVersion;
                });
                if (iter == reqs.end()) {
                    continue;
                }
            }
            return version->descriptor();
        }

        emitFailed(tr("Failed to find version for %1 loader").arg(loaderType));
        return {};
    }

    if (loaderVersion.isEmpty()) {
        emitFailed(tr("No loader version set for modpack!"));
        return {};
    }

    return loaderVersion;
}

void FlameCreationTask::setManagedPack(BaseInstance* instance)
{
    // Don't add managed info to packs without an ID (most likely imported from ZIP)
    if (!m_managedId.isEmpty()) {
        instance->setManagedPack("flame", m_managedId, m_pack.name, m_managedVersionId, m_pack.version);
    } else {
        instance->setManagedPack("flame", "", name(), "", "");
    }
}

void FlameCreationTask::createInstance()
{
    const QString parentFolder(FS::PathCombine(m_stagingPath, "flame"));

    try {
        const QString indexPath(FS::PathCombine(m_stagingPath, "manifest.json"));
        if (!m_pack.isLoaded) {
            Flame::loadManifest(m_pack, indexPath);
        }

        // Keep index file in case we need it some other time (like when changing versions)
        const QString newIndexPlace(FS::PathCombine(parentFolder, "manifest.json"));
        FS::ensureFilePathExists(newIndexPlace);
        FS::move(indexPath, newIndexPlace);

    } catch (const JSONValidationError& e) {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        return;
    }

    if (!m_pack.overrides.isEmpty()) {
        QString overridePath = FS::PathCombine(m_stagingPath, m_pack.overrides);
        if (QFile::exists(overridePath)) {
            // Create a list of overrides in "overrides.txt" inside flame/
            Override::createOverrides("overrides", parentFolder, overridePath);

            QString mcPath = FS::PathCombine(m_stagingPath, "minecraft");
            if (!FS::move(overridePath, mcPath)) {
                emitFailed(tr("Could not rename the overrides folder:\n") + m_pack.overrides);
                return;
            }
        } else {
            logWarning(
                tr("The specified overrides folder (%1) is missing. Maybe the modpack was already used before?").arg(m_pack.overrides));
        }
    }

    QString loaderType;
    QString loaderUid;
    QString loaderVersion;

    for (auto& loader : m_pack.minecraft.modLoaders) {
        auto id = loader.id;
        if (id.startsWith("neoforge-")) {
            id.remove("neoforge-");
            if (id.startsWith("1.20.1-")) {
                id.remove("1.20.1-");  // this is a mess for curseforge
            }
            loaderType = "neoforge";
            loaderUid = "net.neoforged";
        } else if (id.startsWith("forge-")) {
            id.remove("forge-");
            loaderType = "forge";
            loaderUid = "net.minecraftforge";
        } else if (id.startsWith("fabric-")) {
            id.remove("fabric-");
            loaderType = "fabric";
            loaderUid = "net.fabricmc.fabric-loader";
        } else if (id.startsWith("quilt-")) {
            id.remove("quilt-");
            loaderType = "quilt";
            loaderUid = "org.quiltmc.quilt-loader";
        } else {
            logWarning(tr("Unknown mod loader in manifest: %1").arg(id));
            continue;
        }
        loaderVersion = id;
    }

    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(configPath);
    m_newInstance = std::make_unique<MinecraftInstance>(m_globalSettings, std::move(instanceSettings), m_stagingPath);
    auto mcVersion = m_pack.minecraft.version;

    // Hack to correct some 'special sauce'...
    if (mcVersion.endsWith('.')) {
        static const QRegularExpression s_regex("[.]+$");
        mcVersion.remove(s_regex);
        logWarning(tr("Mysterious trailing dots removed from Minecraft version while importing pack."));
    }

    auto* components = m_newInstance->getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", mcVersion, true);
    if (!loaderType.isEmpty()) {
        auto version = getVersionForLoader(loaderUid, loaderType, loaderVersion, mcVersion);
        if (version.isEmpty()) {  // because there are more info in getVersionForLoader the emitFailed is trigered inside it
            return;
        }
        components->setComponentVersion(loaderUid, version);
    }

    if (m_instIcon != "default") {
        m_newInstance->setIconKey(m_instIcon);
    } else {
        if (m_pack.name.contains("Direwolf20")) {
            m_newInstance->setIconKey("steve");
        } else if (m_pack.name.contains("FTB") || m_pack.name.contains("Feed The Beast")) {
            m_newInstance->setIconKey("ftb_logo");
        } else {
            m_newInstance->setIconKey("flame");
        }
    }

    int recommendedRAM = m_pack.minecraft.recommendedRAM;

    // only set memory if this is a fresh instance
    if (!m_oldInstance && recommendedRAM > 0) {
        const uint64_t sysMiB = HardwareInfo::totalRamMiB();
        const uint64_t max = sysMiB * 0.9;

        if (static_cast<uint64_t>(recommendedRAM) > max) {
            logWarning(tr("The recommended memory of the modpack exceeds 90% of your system RAM—reducing it from %1 MiB to %2 MiB!")
                           .arg(recommendedRAM)
                           .arg(max));
            recommendedRAM = max;
        }

        m_newInstance->settings()->set("OverrideMemory", true);
        m_newInstance->settings()->set("MaxMemAlloc", recommendedRAM);
    }

    QString jarmodsPath = FS::PathCombine(m_stagingPath, "minecraft", "jarmods");
    QFileInfo jarmodsInfo(jarmodsPath);
    if (jarmodsInfo.isDir()) {
        // install all the jar mods
        qDebug() << "Found jarmods:";
        QDir jarmodsDir(jarmodsPath);
        QStringList jarMods;
        for (const auto& info : jarmodsDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files)) {
            qDebug() << info.fileName();
            jarMods.push_back(info.absoluteFilePath());
        }
        auto* profile = m_newInstance->getPackProfile();
        profile->installJarMods(jarMods);
        // nuke the original files
        FS::deletePath(jarmodsPath);
    }

    setManagedPack(m_newInstance.get());

    m_newInstance->setName(name());

    m_modIdResolver.reset(new Flame::FileResolvingTask(m_pack));
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::succeeded, this, &FlameCreationTask::idResolverSucceeded);
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::failed, [this](const QString& reason) {
        m_modIdResolver.reset();
        emitFailed(tr("Unable to resolve mod IDs:\n") + reason);
    });
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::aborted, this, &FlameCreationTask::emitAborted);
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::progress, this, &FlameCreationTask::setProgress);
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::status, this, &FlameCreationTask::setStatus);
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::stepProgress, this, &FlameCreationTask::propagateStepProgress);
    connect(m_modIdResolver.get(), &Flame::FileResolvingTask::details, this, &FlameCreationTask::setDetails);
    m_modIdResolver->start();
}

void FlameCreationTask::idResolverSucceeded()
{
    auto results = m_modIdResolver->getResults().files;

    QStringList optionalFiles;
    for (auto& result : results) {
        if (!result.required) {
            optionalFiles << FS::PathCombine(result.targetFolder, result.version.fileName);
        }
    }

    if (!optionalFiles.empty()) {
        OptionalModDialog optionalModDialog(m_parent, optionalFiles);
        if (optionalModDialog.exec() == QDialog::Rejected) {
            emitAborted();
            return;
        }

        m_selectedOptionalMods = optionalModDialog.getResult();
    }

    // first check for blocked mods
    QList<BlockedMod> blockedMods;
    auto anyBlocked = false;
    for (const auto& result : results.values()) {
        if (result.resourceType != ModPlatform::ResourceType::Mod) {
            m_otherResources.append(std::make_pair(result.version.fileName, result.targetFolder));
        }

        // skip optional mods that were not selected
        if (result.version.downloadUrl.isEmpty()) {
            BlockedMod blockedMod;
            blockedMod.name = result.version.fileName;
            blockedMod.websiteUrl = QString("%1/download/%2").arg(result.pack.websiteUrl, QString::number(result.fileId));
            blockedMod.hash = result.version.hash;
            blockedMod.matched = false;
            blockedMod.localPath = "";
            blockedMod.targetFolder = result.targetFolder;
            auto fileName = result.version.fileName;
            fileName = FS::RemoveInvalidPathChars(fileName);
            auto relpath = FS::PathCombine(result.targetFolder, fileName);
            blockedMod.disabled = !result.required && !m_selectedOptionalMods.contains(relpath);

            blockedMods.append(blockedMod);

            anyBlocked = true;
        }
    }
    if (anyBlocked) {
        qWarning() << "Blocked mods found, displaying mod list";

        BlockedModsDialog messageDialog(m_parent, tr("Blocked mods found"),
                                        tr("The following files are not available for download in third party launchers.<br/>"
                                           "You will need to manually download them and add them to the instance."),
                                        blockedMods);

        messageDialog.setModal(true);

        if (messageDialog.exec() != 0) {
            qDebug() << "Post dialog blocked mods list:" << blockedMods;
            copyBlockedMods(blockedMods);
            setupDownloadJob();
        } else {
            m_modIdResolver.reset();
            emitAborted();
            return;
        }
    } else {
        setupDownloadJob();
    }
}

void FlameCreationTask::setupDownloadJob()
{
    m_filesJob.reset(new NetJob(tr("Mod Download Flame"), APPLICATION->network()));
    auto results = m_modIdResolver->getResults().files;

    for (const auto& result : results) {
        auto fileName = result.version.fileName;
        fileName = FS::RemoveInvalidPathChars(fileName);
        auto relpath = FS::PathCombine(result.targetFolder, fileName);

        if (!result.required && !m_selectedOptionalMods.contains(relpath)) {
            relpath += ".disabled";
        }

        relpath = FS::PathCombine("minecraft", relpath);
        auto path = FS::PathCombine(m_stagingPath, relpath);

        if (!result.version.downloadUrl.isEmpty()) {
            qDebug() << "Will download" << result.version.downloadUrl << "to" << path;
            auto dl = Net::ApiDownload::makeFile(result.version.downloadUrl, path);
            m_filesJob->addNetAction(dl);
        }
    }

    connect(m_filesJob.get(), &NetJob::finished, this, [this]() {
        m_filesJob.reset();
        validateOtherResources();
    });
    connect(m_filesJob.get(), &NetJob::failed, [this](QString reason) {
        m_filesJob.reset();
        emitFailed(std::move(reason));
    });
    connect(m_filesJob.get(), &NetJob::progress, this, [this](qint64 current, qint64 total) {
        setDetails(tr("%1 out of %2 complete").arg(current).arg(total));
        setProgress(current, total);
    });
    connect(m_filesJob.get(), &NetJob::stepProgress, this, &FlameCreationTask::propagateStepProgress);

    setStatus(tr("Downloading mods..."));
    m_filesJob->start();
}

/// @brief copy the matched blocked mods to the instance staging area
/// @param blockedMods list of the blocked mods and their matched paths
void FlameCreationTask::copyBlockedMods(const QList<BlockedMod>& blockedMods)
{
    setStatus(tr("Copying Blocked Mods..."));
    setAbortable(false);
    int i = 0;
    const auto total = blockedMods.length();
    setProgress(i, static_cast<int>(total));
    for (const auto& mod : blockedMods) {
        if (!mod.matched) {
            qDebug() << mod.name << "was not matched to a local file, skipping copy";
            continue;
        }

        auto destPath = FS::PathCombine(m_stagingPath, "minecraft", mod.targetFolder, mod.name);
        if (mod.disabled) {
            destPath += ".disabled";
        }

        setStatus(tr("Copying Blocked Mods (%1 out of %2 are done)").arg(QString::number(i), QString::number(total)));

        qDebug() << "Will try to copy" << mod.localPath << "to" << destPath;

        if (mod.move) {
            if (!FS::move(mod.localPath, destPath)) {
                qDebug() << "Move of" << mod.localPath << "to" << destPath << "Failed";
            }
        } else {
            if (!FS::copy(mod.localPath, destPath)()) {
                qDebug() << "Copy of" << mod.localPath << "to" << destPath << "Failed";
            }
        }

        i++;
        setProgress(i, total);
    }

    setAbortable(true);
}

void FlameCreationTask::validateOtherResources()
{
    qDebug() << "Validating whether other resources are in the right place";
    QStringList zipMods;
    for (const auto& [fileName, targetFolder] : m_otherResources) {
        qDebug() << "Checking" << fileName << "...";
        auto localPath = FS::PathCombine(m_stagingPath, "minecraft", targetFolder, fileName);

        /// @brief check the target and move the the file
        /// @return path where file can now be found
        auto validatePath = [&localPath, this](const QString& fileName, const QString& targetFolder, const QString& realTarget) {
            if (targetFolder != realTarget) {
                qDebug() << "Target folder of" << fileName << "is incorrect, it belongs in" << realTarget;
                auto destPath = FS::PathCombine(m_stagingPath, "minecraft", realTarget, fileName);
                qDebug() << "Moving" << localPath << "to" << destPath;
                if (FS::move(localPath, destPath)) {
                    return destPath;
                }
            } else {
                qDebug() << "Target folder of" << fileName << "is correct at" << targetFolder;
            }
            return localPath;
        };

        auto installWorld = [this](const QString& worldPath) {
            qDebug() << "Installing World from" << worldPath;
            QFileInfo worldFileInfo(worldPath);
            World w(worldFileInfo);
            if (!w.isValid()) {
                qDebug() << "World at" << worldPath << "is not valid, skipping install.";
            } else {
                w.install(FS::PathCombine(m_stagingPath, "minecraft", "saves"));
            }
        };

        QFileInfo localFileInfo(localPath);
        auto type = ResourceUtils::identify(localFileInfo);

        QString worldPath;

        switch (type) {
            case ModPlatform::ResourceType::Mod:
                validatePath(fileName, targetFolder, "mods");
                zipMods.push_back(fileName);
                break;
            case ModPlatform::ResourceType::ResourcePack:
                validatePath(fileName, targetFolder, "resourcepacks");
                break;
            case ModPlatform::ResourceType::TexturePack:
                validatePath(fileName, targetFolder, "texturepacks");
                break;
            case ModPlatform::ResourceType::DataPack:
                validatePath(fileName, targetFolder, "datapacks");
                break;
            case ModPlatform::ResourceType::ShaderPack:
                // in theory flame API can't do this but who knows, that *may* change ?
                // better to handle it if it *does* occur in the future
                validatePath(fileName, targetFolder, "shaderpacks");
                break;
            case ModPlatform::ResourceType::World:
                worldPath = validatePath(fileName, targetFolder, "saves");
                installWorld(worldPath);
                break;
            case ModPlatform::ResourceType::Unknown:
            /* fallthrough */
            default:
                qDebug() << "Can't Identify" << fileName << "at" << localPath << ", leaving it where it is.";
                break;
        }
    }
    // TODO make this work with other sorts of resource
    auto task = makeShared<ConcurrentTask>("CreateModMetadata", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    auto results = m_modIdResolver->getResults().files;
    auto folder = FS::PathCombine(m_stagingPath, "minecraft", "mods", ".index");
    for (const auto& file : results) {
        if (file.targetFolder != "mods" || (file.version.fileName.endsWith(".zip") && !zipMods.contains(file.version.fileName))) {
            continue;
        }
        task->addTask(makeShared<LocalResourceUpdateTask>(folder, file.pack, file.version));
    }
    connect(task.get(), &Task::finished, this, &FlameCreationTask::finishInstall);
    m_processUpdateFileInfoJob = task;
    task->start();
}

void FlameCreationTask::finishInstall()
{
    // Update information of the already installed instance, if any.
    if (m_oldInstance) {
        setAbortable(false);
        setManagedPack(m_oldInstance.value_or(nullptr));
    }

    if (shouldOverride()) {
        bool deleteFailed = false;

        setAbortable(false);
        setStatus(tr("Removing old conflicting files..."));
        qDebug() << "Removing old files";

        for (const QString& path : m_filesToRemove) {
            if (!QFile::exists(path)) {
                continue;
            }

            qDebug() << "Removing" << path;

            if (!QFile::remove(path)) {
                qCritical() << "Could not remove" << path;
                deleteFailed = true;
            }
        }

        if (deleteFailed) {
            emitFailed(tr("Failed to remove old conflicting files."));
            return;
        }
    }
    downloadFiles(m_newInstance.get());
}
