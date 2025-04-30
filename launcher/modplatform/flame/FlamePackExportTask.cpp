// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
 */

#include "FlamePackExportTask.h"
#include <QJsonArray>
#include <QJsonObject>

#include <QCryptographicHash>
#include <QFileInfo>
#include <QMessageBox>
#include <QtConcurrentRun>
#include <algorithm>
#include <iterator>
#include <memory>
#include "Application.h"
#include "Json.h"
#include "MMCZip.h"
#include "api/Api.h"
#include "api/structures/Arguments.h"
#include "api/structures/Project.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/helpers/HashUtils.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

const QString FlamePackExportTask::TEMPLATE = "<li><a href=\"{url}\">{name}{authors}</a></li>\n";
const QStringList FlamePackExportTask::FILE_EXTENSIONS({ "jar", "zip" });

FlamePackExportTask::FlamePackExportTask(const QString& name,
                                         const QString& version,
                                         const QString& author,
                                         bool optionalFiles,
                                         InstancePtr instance,
                                         const QString& output,
                                         MMCZip::FilterFileFunction filter)
    : name(name)
    , version(version)
    , author(author)
    , optionalFiles(optionalFiles)
    , instance(instance)
    , mcInstance(dynamic_cast<MinecraftInstance*>(instance.get()))
    , gameRoot(instance->gameRoot())
    , output(output)
    , filter(filter)
{}

void FlamePackExportTask::executeTask()
{
    setStatus(tr("Searching for files..."));
    setProgress(0, 5);
    collectFiles();
}

bool FlamePackExportTask::abort()
{
    if (task) {
        task->abort();
        return true;
    }
    return false;
}

void FlamePackExportTask::collectFiles()
{
    setAbortable(false);
    QCoreApplication::processEvents();

    files.clear();
    if (!MMCZip::collectFileListRecursively(instance->gameRoot(), nullptr, &files, filter)) {
        emitFailed(tr("Could not search for files"));
        return;
    }

    pendingHashes.clear();
    resolvedFiles.clear();

    if (mcInstance != nullptr) {
        mcInstance->loaderModList()->update();
        connect(mcInstance->loaderModList().get(), &ModFolderModel::updateFinished, this, &FlamePackExportTask::collectHashes);
    } else
        collectHashes();
}

void FlamePackExportTask::collectHashes()
{
    setAbortable(true);
    setStatus(tr("Finding file hashes..."));
    setProgress(1, 5);
    auto allMods = mcInstance->loaderModList()->allMods();
    ConcurrentTask::Ptr hashingTask(new ConcurrentTask("MakeHashesTask", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt()));
    task.reset(hashingTask);
    for (const QFileInfo& file : files) {
        const QString relative = gameRoot.relativeFilePath(file.absoluteFilePath());
        // require sensible file types
        if (!std::any_of(FILE_EXTENSIONS.begin(), FILE_EXTENSIONS.end(), [&relative](const QString& extension) {
                return relative.endsWith('.' + extension) || relative.endsWith('.' + extension + ".disabled");
            }))
            continue;

        if (relative.startsWith("resourcepacks/") &&
            (relative.endsWith(".zip") || relative.endsWith(".zip.disabled"))) {  // is resourcepack
            auto hashTask = Hashing::createHasher(file.absoluteFilePath(), Platform::Provider::FLAME);
            connect(hashTask.get(), &Hashing::Hasher::resultsReady, [this, relative, file](QString hash) {
                if (m_state == Task::State::Running) {
                    pendingHashes.insert(hash, { relative, file.absoluteFilePath(), relative.endsWith(".zip") });
                }
            });
            connect(hashTask.get(), &Task::failed, this, &FlamePackExportTask::emitFailed);
            hashingTask->addTask(hashTask);
            continue;
        }

        if (auto modIter = std::find_if(allMods.begin(), allMods.end(), [&file](Mod* mod) { return mod->fileinfo() == file; });
            modIter != allMods.end()) {
            const Mod* mod = *modIter;
            if (!mod || mod->type() == ResourceType::FOLDER) {
                continue;
            }
            if (mod->metadata() && mod->metadata()->provider == Platform::Provider::FLAME) {
                resolvedFiles.insert(mod->fileinfo().absoluteFilePath(),
                                     { mod->metadata()->project_id.toInt(), mod->metadata()->file_id.toInt(), mod->enabled(), true,
                                       mod->metadata()->name, mod->metadata()->slug, mod->authors().join(", ") });
                continue;
            }

            auto hashTask = Hashing::createHasher(mod->fileinfo().absoluteFilePath(), Platform::Provider::FLAME);
            connect(hashTask.get(), &Hashing::Hasher::resultsReady, [this, mod](QString hash) {
                if (m_state == Task::State::Running) {
                    pendingHashes.insert(hash, { mod->name(), mod->fileinfo().absoluteFilePath(), mod->enabled(), true });
                }
            });
            connect(hashTask.get(), &Task::failed, this, &FlamePackExportTask::emitFailed);
            hashingTask->addTask(hashTask);
        }
    }
    auto progressStep = std::make_shared<TaskStepProgress>();
    connect(hashingTask.get(), &Task::finished, this, [this, progressStep] {
        progressStep->state = TaskStepState::Succeeded;
        stepProgress(*progressStep);
    });

    connect(hashingTask.get(), &Task::succeeded, this, &FlamePackExportTask::makeApiRequest);
    connect(hashingTask.get(), &Task::failed, this, [this, progressStep](QString reason) {
        progressStep->state = TaskStepState::Failed;
        stepProgress(*progressStep);
        emitFailed(reason);
    });
    connect(hashingTask.get(), &Task::stepProgress, this, &FlamePackExportTask::propagateStepProgress);

    connect(hashingTask.get(), &Task::progress, this, [this, progressStep](qint64 current, qint64 total) {
        progressStep->update(current, total);
        stepProgress(*progressStep);
    });
    connect(hashingTask.get(), &Task::status, this, [this, progressStep](QString status) {
        progressStep->status = status;
        stepProgress(*progressStep);
    });
    connect(hashingTask.get(), &Task::aborted, this, &FlamePackExportTask::emitAborted);
    hashingTask->start();
}

void FlamePackExportTask::makeApiRequest()
{
    if (pendingHashes.isEmpty()) {
        buildZip();
        return;
    }

    setStatus(tr("Finding versions for hashes..."));
    setProgress(2, 5);
    auto response = std::make_shared<API::MatchHashesResponse>();

    QStringList fingerprints = pendingHashes.keys();

    auto hashTask = API::ProviderAPI::get(Platform::Provider::FLAME)->makeMatchHashesRequest({ fingerprints }, response);

    auto ver_task = makeShared<NetJob>(QString("Flame::GetHashes"), APPLICATION->network());
    ver_task->addNetAction(hashTask);
    ver_task->setAskRetry(false);

    task.reset(ver_task);

    connect(task.get(), &Task::succeeded, this, [this, response] {
        if (response->isEmpty()) {
            qWarning() << "No matches found for fingerprint search!";

            getProjectsInfo();
            return;
        }
        for (auto fingerprint : response->keys()) {
            auto mod = pendingHashes.find(fingerprint);
            if (mod == pendingHashes.end()) {
                qWarning() << "Invalid fingerprint from the API response.";
                continue;
            }

            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(mod->name));

            auto file = response->value(fingerprint);
            if (file.isAvailable)
                resolvedFiles.insert(mod->path, { file.projectId.toInt(), file.fileId.toInt(), mod->enabled, mod->isMod });
        }

        pendingHashes.clear();
        getProjectsInfo();
    });
    connect(task.get(), &Task::failed, this, &FlamePackExportTask::getProjectsInfo);
    connect(task.get(), &Task::aborted, this, &FlamePackExportTask::emitAborted);
    task->start();
}

void FlamePackExportTask::getProjectsInfo()
{
    setStatus(tr("Finding project info from CurseForge..."));
    setProgress(3, 5);
    QStringList addonIds;
    for (const auto& resolved : resolvedFiles) {
        if (resolved.slug.isEmpty()) {
            addonIds << QString::number(resolved.addonId);
        }
    }

    Net::NetRequest::Ptr projTask;
    auto response = std::make_shared<Platform::Project>();
    auto responses = std::make_shared<QList<Platform::Project::Ptr>>();

    auto api = API::ProviderAPI::get(Platform::Provider::FLAME);
    if (addonIds.isEmpty()) {
        buildZip();
        return;
    } else if (addonIds.size() == 1) {
        projTask = api->makeGetProjectRequest(*addonIds.begin(), response);
    } else {
        projTask = api->makeGetProjectsRequest(addonIds, responses);
    }
    auto netJob = makeShared<NetJob>(QString("Flame::GetProjects"), APPLICATION->network());
    netJob->addNetAction(projTask);

    connect(netJob.get(), &Task::succeeded, this, [this, response, responses, addonIds] {
        auto update = [this](Platform::Project::Ptr response) {
            auto id = response->projectId.toString();
            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(response->name));
            for (auto key : resolvedFiles.keys()) {
                auto val = resolvedFiles.value(key);
                if (val.addonId == response->projectId) {
                    val.name = response->name;
                    val.slug = response->slug;
                    QStringList authors;
                    for (auto author : response->authors)
                        authors << author.name;

                    val.authors = authors.join(", ");
                    resolvedFiles[key] = val;
                }
            }
        };
        if (addonIds.size() == 1) {
            update(response);
        } else {
            for (auto respone : *responses) {
                update(response);
            }
        }
        buildZip();
    });
    connect(netJob.get(), &Task::failed, this, &FlamePackExportTask::emitFailed);
    connect(netJob.get(), &Task::aborted, this, &FlamePackExportTask::emitAborted);
    task.reset(netJob);
    task->start();
}

void FlamePackExportTask::buildZip()
{
    setStatus(tr("Adding files..."));
    setProgress(4, 5);

    auto zipTask = makeShared<MMCZip::ExportToZipTask>(output, gameRoot, files, "overrides/", true, false);
    zipTask->addExtraFile("manifest.json", generateIndex());
    zipTask->addExtraFile("modlist.html", generateHTML());

    QStringList exclude;
    std::transform(resolvedFiles.keyBegin(), resolvedFiles.keyEnd(), std::back_insert_iterator(exclude),
                   [this](QString file) { return gameRoot.relativeFilePath(file); });
    zipTask->setExcludeFiles(exclude);

    auto progressStep = std::make_shared<TaskStepProgress>();
    connect(zipTask.get(), &Task::finished, this, [this, progressStep] {
        progressStep->state = TaskStepState::Succeeded;
        stepProgress(*progressStep);
    });

    connect(zipTask.get(), &Task::succeeded, this, &FlamePackExportTask::emitSucceeded);
    connect(zipTask.get(), &Task::aborted, this, &FlamePackExportTask::emitAborted);
    connect(zipTask.get(), &Task::failed, this, [this, progressStep](QString reason) {
        progressStep->state = TaskStepState::Failed;
        stepProgress(*progressStep);
        emitFailed(reason);
    });
    connect(zipTask.get(), &Task::stepProgress, this, &FlamePackExportTask::propagateStepProgress);

    connect(zipTask.get(), &Task::progress, this, [this, progressStep](qint64 current, qint64 total) {
        progressStep->update(current, total);
        stepProgress(*progressStep);
    });
    connect(zipTask.get(), &Task::status, this, [this, progressStep](QString status) {
        progressStep->status = status;
        stepProgress(*progressStep);
    });
    task.reset(zipTask);
    zipTask->start();
}

QByteArray FlamePackExportTask::generateIndex()
{
    QJsonObject obj;
    obj["manifestType"] = "minecraftModpack";
    obj["manifestVersion"] = 1;
    obj["name"] = name;
    obj["version"] = version;
    obj["author"] = author;
    obj["overrides"] = "overrides";
    if (mcInstance) {
        QJsonObject version;
        auto profile = mcInstance->getPackProfile();
        // collect all supported components
        const ComponentPtr minecraft = profile->getComponent("net.minecraft");
        const ComponentPtr quilt = profile->getComponent("org.quiltmc.quilt-loader");
        const ComponentPtr fabric = profile->getComponent("net.fabricmc.fabric-loader");
        const ComponentPtr forge = profile->getComponent("net.minecraftforge");
        const ComponentPtr neoforge = profile->getComponent("net.neoforged");

        // convert all available components to mrpack dependencies
        if (minecraft != nullptr)
            version["version"] = minecraft->m_version;
        QString id;
        if (quilt != nullptr)
            id = "quilt-" + quilt->m_version;
        else if (fabric != nullptr)
            id = "fabric-" + fabric->m_version;
        else if (forge != nullptr)
            id = "forge-" + forge->m_version;
        else if (neoforge != nullptr) {
            id = "neoforge-";
            if (minecraft->m_version == "1.20.1")
                id += "1.20.1-";
            id += neoforge->m_version;
        }
        version["modLoaders"] = QJsonArray();
        if (!id.isEmpty()) {
            QJsonObject loader;
            loader["id"] = id;
            loader["primary"] = true;
            version["modLoaders"] = QJsonArray({ loader });
        }
        obj["minecraft"] = version;
    }

    QJsonArray files;
    for (auto mod : resolvedFiles) {
        QJsonObject file;
        file["projectID"] = mod.addonId;
        file["fileID"] = mod.version;
        file["required"] = mod.enabled || !optionalFiles;
        files << file;
    }
    obj["files"] = files;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QByteArray FlamePackExportTask::generateHTML()
{
    QString content = "";
    for (auto mod : resolvedFiles) {
        if (mod.isMod) {
            content += QString(TEMPLATE)
                           .replace("{name}", mod.name.toHtmlEscaped())
                           .replace("{url}", Platform::ProviderUtils::getMetaURL(Platform::Provider::FLAME, mod.addonId).toHtmlEscaped())
                           .replace("{authors}", !mod.authors.isEmpty() ? QString(" (by %1)").arg(mod.authors).toHtmlEscaped() : "");
        }
    }
    content = "<ul>" + content + "</ul>";
    return content.toUtf8();
}