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

#include "config/GlobalConfig.h"
#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"

#include "modplatform/modrinth/ModrinthPackIndex.h"
#include "net/NetJob.h"
#include "settings/SettingsObject.h"
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
    for (const auto& file : m_manifest.files) {
        fileIds.push_back(QString::number(file.fileId));
    }
    auto [task, response] = FlameAPI::getFiles(fileIds);
    m_task = task;

    auto stepProgress2 = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, response, stepProgress2]() {
        stepProgress2->state = TaskStepState::Succeeded;
        stepProgress(*stepProgress2);
        netJobFinished(response);
    });
    connect(m_task.get(), &Task::failed, this, [this, stepProgress2](QString reason) {
        stepProgress2->state = TaskStepState::Failed;
        stepProgress(*stepProgress2);
        emitFailed(std::move(reason));
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, stepProgress2](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        stepProgress2->update(current, total);
        stepProgress(*stepProgress2);
    });
    connect(m_task.get(), &Task::status, this, [this, stepProgress2](QString status) {
        stepProgress2->status = std::move(status);
        stepProgress(*stepProgress2);
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
            if (!url.isValid() && "sha1" == version.hashType && !version.hash.isEmpty()) {
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
    auto [modrinthTask, modrinthResponse] = ModrinthAPI::currentVersions(hashes, "sha1");
    m_task = modrinthTask;
    (dynamic_cast<NetJob*>(m_task.get()))->setAskRetry(false);
    auto stepProgress2 = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, modrinthResponse, stepProgress2]() {
        stepProgress2->state = TaskStepState::Succeeded;
        stepProgress(*stepProgress2);
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(*modrinthResponse, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth::CurrentVersions at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << *modrinthResponse;

            getFlameProjects();
            return;
        }
        if (APPLICATION->config()->fallbackModrinthBlockedMods) {
            try {
                auto entries = Json::requireObject(doc);
                for (auto& out : m_manifest.files) {
                    auto url = QUrl(out.version.downloadUrl, QUrl::TolerantMode);
                    if (!url.isValid() && "sha1" == out.version.hashType && !out.version.hash.isEmpty()) {
                        try {
                            auto entry = Json::requireObject(entries, out.version.hash);

                            auto file = Modrinth::loadIndexedPackVersion(entry);

                            out.version.downloadUrl = file.downloadUrl;
                            qDebug() << "Found alternative on modrinth" << out.version.fileName;
                        } catch (Json::JsonException& e) {
                            qDebug() << e.cause();
                            qDebug() << entries;
                        }
                    }
                }
            } catch (Json::JsonException& e) {
                qDebug() << e.cause();
                qDebug() << doc;
            }
        }
        getFlameProjects();
    });
    connect(m_task.get(), &Task::failed, this, [this, stepProgress2](const QString& /*reason*/) {
        stepProgress2->state = TaskStepState::Failed;
        stepProgress(*stepProgress2);
        getFlameProjects();
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, stepProgress2](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        stepProgress2->update(current, total);
        stepProgress(*stepProgress2);
    });
    connect(m_task.get(), &Task::status, this, [this, stepProgress2](QString status) {
        stepProgress2->status = std::move(status);
        stepProgress(*stepProgress2);
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

    auto [task, response] = FlameAPI::get().getProjects(addonIds);
    m_task = task;

    auto stepProgress2 = std::make_shared<TaskStepProgress>();
    connect(m_task.get(), &Task::succeeded, this, [this, response, stepProgress2] {
        QJsonParseError parseError{};
        auto doc = QJsonDocument::fromJson(*response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth projects task at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << *response;
            return;
        }

        try {
            QJsonArray entries;
            entries = Json::requireArray(Json::requireObject(doc), "data");

            for (auto entry : entries) {
                auto entryObj = Json::requireObject(entry);
                auto id = Json::requireInteger(entryObj, "id");
                auto file = std::find_if(m_manifest.files.begin(), m_manifest.files.end(),
                                         [id](const Flame::File& file) { return file.projectId == id; });
                if (file == m_manifest.files.end()) {
                    continue;
                }

                setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(file->version.fileName));
                FlameMod::loadIndexedPack(file->pack, entryObj);
                if (file->pack.resourceType == ModPlatform::ResourceType::World) {
                    file->targetFolder = "saves";
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
        stepProgress2->state = TaskStepState::Succeeded;
        stepProgress(*stepProgress2);
        emitSucceeded();
    });

    connect(m_task.get(), &Task::failed, this, [this, stepProgress2](QString reason) {
        stepProgress2->state = TaskStepState::Failed;
        stepProgress(*stepProgress2);
        emitFailed(std::move(reason));
    });
    connect(m_task.get(), &Task::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_task.get(), &Task::progress, this, [this, stepProgress2](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        stepProgress2->update(current, total);
        stepProgress(*stepProgress2);
    });
    connect(m_task.get(), &Task::status, this, [this, stepProgress2](QString status) {
        stepProgress2->status = std::move(status);
        stepProgress(*stepProgress2);
    });

    m_task->start();
}
