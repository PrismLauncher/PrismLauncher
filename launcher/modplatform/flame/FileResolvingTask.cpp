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

    auto doc = Json::requireDocument(*response).and_then([](const auto& v) { return Json::requireArray(v.object()["data"]); });
    if (!doc) {
        qCritical() << "Failed to parse CurseForge files response";
        qCritical() << "Parse error:" << doc.error();

        emitFailed(tr("Invalid data returned from the API."));

        return;
    }

    QStringList hashes;
    for (QJsonValueRef file : doc.value()) {
        auto process = [this, &hashes](const QJsonValue& file) -> Result<> {
            auto obj = Json::requireObject(file);
            TRY(obj)
            auto versionRes = FlameMod::loadIndexedPackVersion(obj.value());
            TRY(versionRes)
            auto& version = versionRes.value();
            auto fileid = version.fileId.toInt();
            Q_ASSERT(fileid != 0);
            Q_ASSERT(m_manifest.files.contains(fileid));
            m_manifest.files[fileid].version = version;
            auto url = QUrl(version.downloadUrl, QUrl::TolerantMode);
            if (!url.isValid() && "sha1" == version.hashType && !version.hash.isEmpty()) {
                hashes.push_back(version.hash);
            }
            return {};
        };
        if (auto result = process(file); !result) {
            qCritical() << "Failed to parse CurseForge file entry";
            qCritical() << "Parse error:" << result.error();

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

        auto doc = Json::requireObject(*modrinthResponse);
        if (!doc) {
            qWarning() << "Error while parsing JSON response from Modrinth::CurrentVersions:" << doc.error();
            qWarning() << *modrinthResponse;

            getFlameProjects();
            return;
        }
        if (APPLICATION->settings()->get("FallbackMRBlockedMods").toBool()) {
            const auto& entries = doc.value();
            for (auto& out : m_manifest.files) {
                auto url = QUrl(out.version.downloadUrl, QUrl::TolerantMode);
                if (!url.isValid() && "sha1" == out.version.hashType && !out.version.hash.isEmpty()) {
                    auto parse = [&entries, &out]() -> Result<> {
                        auto entry = Json::requireObject(entries, out.version.hash);
                        TRY(entry)

                        auto file = Modrinth::loadIndexedPackVersion(entry.value());
                        TRY(file)

                        out.version.downloadUrl = file->downloadUrl;
                        qDebug() << "Found alternative on modrinth" << out.version.fileName;
                        return {};
                    };
                    if (auto rsp = parse(); !rsp) {
                        qDebug() << rsp.error();
                        qDebug() << entries;
                        continue;
                    }
                }
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
        auto doc = Json::requireObject(*response).and_then([](const auto& v) {
            return Json::requireArray(v, "data");
        });
        if (!doc) {
            qWarning() << "Error while parsing CurseForge projects response:" << doc.error();
            qWarning() << *response;
            // treat a parse failure as success, otherwise the task hangs forever
            stepProgress2->state = TaskStepState::Succeeded;
            stepProgress(*stepProgress2);
            emitSucceeded();
            return;
        }

        for (auto entry : doc.value()) {
            auto process = [this, &entry]() -> Result<> {
                auto entryObj = Json::requireObject(entry);
                TRY(entryObj)
                auto id = Json::requireInteger(entryObj.value(), "id");
                TRY(id)

                auto file = std::ranges::find_if(m_manifest.files, [id](const Flame::File& file) { return file.projectId == id.value(); });
                if (file != m_manifest.files.end()) {
                    setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(file->version.fileName));
                    auto loadRes = FlameMod::loadIndexedPack(file->pack, entryObj.value());
                    TRY(loadRes)
                    if (file->pack.resourceType == ModPlatform::ResourceType::World) {
                        file->targetFolder = "saves";
                    }
                }
                return {};
            };
            if (auto result = process(); !result) {
                qDebug() << result.error();
                qDebug() << *doc;
                break;
            }
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
