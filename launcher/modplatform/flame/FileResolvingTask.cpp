// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include "FileResolvingTask.h"
#include <algorithm>

#include "Application.h"
#include "Json.h"
#include "api/Api.h"
#include "api/structures/Project.h"
#include "api/structures/Provider.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"

#include "modplatform/modrinth/ModrinthPackIndex.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

static const FlameAPI flameAPI;

Flame::FileResolvingTask::FileResolvingTask(Flame::Manifest& toProcess) : m_manifest(toProcess) {}

bool Flame::FileResolvingTask::abort()
{
    bool aborted = true;
    if (m_task) {
        aborted = m_task->abort();
    }
    return aborted ? Task::abort() : false;
}

void Flame::FileResolvingTask::executeTask()
{
    if (m_manifest.files.isEmpty()) {  // no file to resolve so leave it empty and emit success immediately
        emitSucceeded();
        return;
    }
    setStatus(tr("Resolving mod IDs..."));
    setProgress(0, 3);
    m_result2.reset(new QByteArray());

    QStringList fileIds;
    for (auto file : m_manifest.files) {
        fileIds.push_back(QString::number(file.fileId));
    }
    m_task = flameAPI.getFiles(fileIds, m_result2);

    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::finished, this, [this, step_progress]() {
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        netJobFinished();
    });
    connect(m_task.get(), &Task::failed, this, [this, step_progress](QString reason) {
        step_progress->state = TaskStepState::Failed;
        stepProgress(*step_progress);
        emitFailed(reason);
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, step_progress](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        step_progress->update(current, total);
        stepProgress(*step_progress);
    });
    connect(m_task.get(), &Task::status, this, [this, step_progress](QString status) {
        step_progress->status = status;
        stepProgress(*step_progress);
    });

    m_task->start();
}

void Flame::FileResolvingTask::netJobFinished()
{
    setProgress(1, 3);
    // job to check modrinth for blocked projects
    QJsonDocument doc;
    QJsonArray array;

    try {
        doc = Json::requireDocument(*m_result2);
        array = Json::requireArray(doc.object()["data"]);
    } catch (Json::JsonException& e) {
        qCritical() << "Non-JSON data returned from the CF API";
        qCritical() << e.cause();

        emitFailed(tr("Invalid data returned from the API."));

        return;
    }

    QStringList hashes;
    for (QJsonValueRef file : array) {
        try {
            auto obj = Json::requireObject(file);
            auto version = FlameMod::loadIndexedPackVersion(obj);
            auto fileid = version.fileId.toInt();
            Q_ASSERT(fileid != 0);
            Q_ASSERT(m_manifest.files.contains(fileid));
            m_manifest.files[fileid].version = version;
            auto url = QUrl(version.downloadUrl, QUrl::TolerantMode);
            if (!url.isValid()) {
                for (auto hash : version.hashes) {
                    if (hash.alg == Hashing::Algorithm::Sha1 && !hash.hash.isEmpty()) {
                        hashes.push_back(hash.hash);
                    }
                }
            }
        } catch (Json::JsonException& e) {
            qCritical() << "Non-JSON data returned from the CF API";
            qCritical() << e.cause();

            emitFailed(tr("Invalid data returned from the API."));

            return;
        }
    }
    if (hashes.isEmpty()) {
        getFlameProjects();
        return;
    }

    auto response = std::make_shared<API::MatchHashesResponse>();
    auto ver_task =
        API::ProviderAPI::get(Platform::Provider::MODRINTH)->makeMatchHashesRequest({ hashes, Hashing::Algorithm::Sha1 }, response);
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetHashes"), APPLICATION->network());
    netJob->addNetAction(ver_task);

    netJob->setAskRetry(false);
    m_task = netJob;
    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::finished, this, [this, step_progress, response]() {
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        for (auto& out : m_manifest.files) {
            auto url = QUrl(out.version.downloadUrl, QUrl::TolerantMode);
            if (!url.isValid()) {
                for (auto hash : out.version.hashes) {
                    if (hash.alg == Hashing::Algorithm::Sha1 && !hash.hash.isEmpty()) {
                        if (response->contains(hash.hash)) {
                            auto file = response->value(hash.hash);

                            // If there's more than one mod loader for this version, we can't know for sure
                            // which file is relative to each loader, so it's best to not use any one and
                            // let the user download it manually.
                            if (!file.loaders || Platform::ModloaderUtils::hasSingleSelected(file.loaders)) {
                                out.version.downloadUrl = file.downloadUrl;
                                qDebug() << "Found alternative on modrinth " << out.version.fileName;
                            }
                        }
                    }
                }
            }
        }
        getFlameProjects();
    });
    connect(m_task.get(), &Task::failed, this, [this, step_progress](QString reason) {
        step_progress->state = TaskStepState::Failed;
        stepProgress(*step_progress);
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, step_progress](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        step_progress->update(current, total);
        stepProgress(*step_progress);
    });
    connect(m_task.get(), &Task::status, this, [this, step_progress](QString status) {
        step_progress->status = status;
        stepProgress(*step_progress);
    });

    m_task->start();
}

void Flame::FileResolvingTask::getFlameProjects()
{
    setProgress(2, 3);
    m_result.reset(new QList<Platform::Project::Ptr>());
    QStringList addonIds;
    for (auto file : m_manifest.files) {
        addonIds.push_back(QString::number(file.projectId));
    }

    auto task = API::ProviderAPI::get(Platform::Provider::FLAME)->makeGetProjectsRequest(addonIds, m_result);
    m_task = makeShared<NetJob>(QString("Flame::GetProjects"), APPLICATION->network());
    m_task->addNetAction(task);

    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, step_progress] {
        for (auto response : *m_result) {
            auto id = response->projectId;
            auto file = std::find_if(m_manifest.files.begin(), m_manifest.files.end(),
                                     [id](const Flame::File& file) { return file.projectId == id; });
            if (file == m_manifest.files.end()) {
                continue;
            }
            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(file->version.fileName));
            if (file->resourceType == Platform::ResourceType::World) {
                file->targetFolder = "saves";
            }
            file->pack = response;
        }

        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        emitSucceeded();
    });

    connect(m_task.get(), &Task::failed, this, [this, step_progress](QString reason) {
        step_progress->state = TaskStepState::Failed;
        stepProgress(*step_progress);
        emitFailed(reason);
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, step_progress](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        step_progress->update(current, total);
        stepProgress(*step_progress);
    });
    connect(m_task.get(), &Task::status, this, [this, step_progress](QString status) {
        step_progress->status = status;
        stepProgress(*step_progress);
    });

    m_task->start();
}
