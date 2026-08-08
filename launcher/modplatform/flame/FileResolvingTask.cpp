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
#include <utility>

#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"

#include "modplatform/modrinth/ModrinthPackIndex.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

#include "Application.h"

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

    QStringList fileIds;
    for (auto file : m_manifest.files) {
        fileIds.push_back(QString::number(file.fileId));
    }
    auto [task, response] = FlameAPI::get().getFiles(fileIds);
    m_task = task;

    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, response, step_progress]() {
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        netJobFinished(response);
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

void Flame::FileResolvingTask::netJobFinished(QByteArray* response)
{
    setProgress(1, 3);
    // job to check modrinth for blocked projects
    QJsonDocument doc;
    QJsonArray array;

    try {
        doc = Json::requireDocument(*response);
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
            if (!url.isValid() && "sha1" == version.hash_type && !version.hash.isEmpty()) {
                hashes.push_back(version.hash);
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
    auto [modrinthTask, result] = ModrinthAPI::get().currentVersions(hashes, "sha1").make();
    m_task = modrinthTask;
    (dynamic_cast<NetJob*>(m_task.get()))->setAskRetry(false);
    auto stepProgressV = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, result, stepProgressV]() {
        stepProgressV->state = TaskStepState::Succeeded;
        stepProgress(*stepProgressV);
        if (APPLICATION->settings()->get("FallbackMRBlockedMods").toBool()) {
            for (auto& out : m_manifest.files) {
                auto url = QUrl(out.version.downloadUrl, QUrl::TolerantMode);
                if (!url.isValid() && "sha1" == out.version.hash_type && !out.version.hash.isEmpty()) {
                    auto it = result->find(out.version.hash);
                    if (it != result->end()) {
                        out.version.downloadUrl = it->downloadUrl;
                        qDebug() << "Found alternative on modrinth" << out.version.fileName;
                    }
                }
            }
        }
        getFlameProjects();
    });
    connect(m_task.get(), &Task::failed, this, [this, stepProgressV](const QString& /*reason*/) {
        stepProgressV->state = TaskStepState::Failed;
        stepProgress(*stepProgressV);
        getFlameProjects();
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, stepProgressV](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        stepProgressV->update(current, total);
        stepProgress(*stepProgressV);
    });
    connect(m_task.get(), &Task::status, this, [this, stepProgressV](QString status) {
        stepProgressV->status = std::move(status);
        stepProgress(*stepProgressV);
    });
    m_task->start();
}

void Flame::FileResolvingTask::getFlameProjects()
{
    setProgress(2, 3);
    QStringList addonIds;
    for (const auto& file : m_manifest.files) {
        addonIds.push_back(QString::number(file.projectId));
    }

    auto [task, result] = FlameAPI::get().getProjects(addonIds).make();
    m_task = task;

    auto stepProgressV = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, result, stepProgressV] {
        for (const auto& pack : *result) {
            auto id = pack->addonId.toInt();
            auto file = std::find_if(m_manifest.files.begin(), m_manifest.files.end(),
                                     [id](const Flame::File& file) { return file.projectId == id; });
            if (file == m_manifest.files.end()) {
                continue;
            }

            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(file->version.fileName));
            file->pack = *pack;
            file->resourceType = pack->resourceType;
            if (file->resourceType == ModPlatform::ResourceType::World) {
                file->targetFolder = "saves";
            }
        }
        stepProgressV->state = TaskStepState::Succeeded;
        stepProgress(*stepProgressV);
        emitSucceeded();
    });

    connect(m_task.get(), &Task::failed, this, [this, stepProgressV](QString reason) {
        stepProgressV->state = TaskStepState::Failed;
        stepProgress(*stepProgressV);
        emitFailed(std::move(reason));
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, stepProgressV](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        stepProgressV->update(current, total);
        stepProgress(*stepProgressV);
    });
    connect(m_task.get(), &Task::status, this, [this, stepProgressV](QString status) {
        stepProgressV->status = std::move(status);
        stepProgress(*stepProgressV);
    });

    m_task->start();
}
