// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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

#include "api/Api.h"
namespace API {

QString ProviderAPI::waitForModFileChangelog(QVariant modId, QVariant fileId) const
{
    QEventLoop lock;

    auto response = std::make_shared<QString>();
    auto netJob = makeGetFileChangelogRequest({ modId, fileId }, response, { QString("FileChangelog::%1").arg(fileId.toString()) });
    if (netJob) {
        QObject::connect(netJob.get(), &NetJob::finished, [&lock] { lock.quit(); });

        netJob->start();
        lock.exec();
    }

    return *response;
}
Task::Ptr ProviderAPI::searchProjects(SearchArgs&& args, Callback<QList<Platform::Project::Ptr>>&& callbacks) const
{
    std::shared_ptr<QList<Platform::Project::Ptr>> newList = std::make_shared<QList<Platform::Project::Ptr>>();
    auto netJob = makeSearchRequest(args, newList, { "Search" });
    if (!netJob) {
        callbacks.on_fail("Failed to create search URL", -1);
        return nullptr;
    }

    QObject::connect(netJob.get(), &NetJob::succeeded, [newList, callbacks] { callbacks.on_succeed(*newList); });

    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = netJob.toWeakRef();
    QObject::connect(netJob.get(), &NetJob::failed, [weak, callbacks](const QString& reason) {
        int network_error_code = -1;
        if (auto netJob = weak.lock()) {
            if (auto* failed_action = netJob->getFailedActions().at(0); failed_action)
                network_error_code = failed_action->replyStatusCode();
        }
        callbacks.on_fail(reason, network_error_code);
    });
    QObject::connect(netJob.get(), &NetJob::aborted, [callbacks] { callbacks.on_abort(); });

    return netJob;
}
Task::Ptr ProviderAPI::getProjectVersions(VersionSearchArgs&& args, Callback<QVector<Platform::Version>>&& callbacks) const
{
    auto response = std::make_shared<VersionSearchResponse>();
    response->projectId = args.pack.projectId;
    response->resourceType = args.resourceType;
    auto netJob = makeGetVersionsRequest(args, response, { QString("Versions::%1").arg(args.pack.name) });

    QObject::connect(netJob.get(), &NetJob::succeeded, [response, callbacks] { callbacks.on_succeed(response->versions); });

    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = netJob.toWeakRef();
    QObject::connect(netJob.get(), &NetJob::failed, [weak, callbacks](const QString& reason) {
        int network_error_code = -1;
        if (auto netJob = weak.lock()) {
            if (auto* failed_action = netJob->getFailedActions().at(0); failed_action)
                network_error_code = failed_action->replyStatusCode();
        }
        callbacks.on_fail(reason, network_error_code);
    });
    QObject::connect(netJob.get(), &NetJob::aborted, [callbacks] { callbacks.on_abort(); });

    return netJob;
}
Task::Ptr ProviderAPI::getProjectInfo(ProjectInfoArgs&& args, Callback<Platform::Project>&& callbacks) const
{
    auto response = args.pack;
    auto projectId = args.pack->projectId.toString();
    auto job = makeGetProjectRequest(projectId, response, { QString("GetProject::%1").arg(projectId), 1 });
    auto descriptionRequest = makeGetDescriptionRequest(projectId, response);
    if (descriptionRequest) {
        job->addNetAction(descriptionRequest);
    }

    QObject ::connect(job.get(), &NetJob::succeeded, [response, callbacks] { callbacks.on_succeed(*response); });
    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = job.toWeakRef();
    QObject::connect(job.get(), &NetJob::failed, [weak, callbacks](const QString& reason) {
        int network_error_code = -1;
        if (auto job = weak.lock()) {
            if (auto netJob = qSharedPointerDynamicCast<NetJob>(job)) {
                if (auto* failed_action = netJob->getFailedActions().at(0); failed_action) {
                    network_error_code = failed_action->replyStatusCode();
                }
            }
        }
        callbacks.on_fail(reason, network_error_code);
    });
    QObject::connect(job.get(), &NetJob::aborted, [callbacks] { callbacks.on_abort(); });
    return job;
}
Task::Ptr ProviderAPI::getDependencyVersion(DependencySearchArgs&& args, Callback<Platform::Version>&& callbacks) const
{
    auto response = std::make_shared<VersionSearchResponse>();
    response->projectId = args.dependency.projectId;
    response->resourceType = Platform::ResourceType::Mod;
    auto netJob = makeGetDependencyRequest(args, response, { QString("Dependency::%1").arg(args.dependency.projectId.toString()) });

    QObject::connect(netJob.get(), &NetJob::succeeded, [response, callbacks] {
        auto bestMatch = response->versions.size() != 0 ? response->versions.front() : Platform::Version();
        callbacks.on_succeed(bestMatch);
    });

    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = netJob.toWeakRef();
    QObject::connect(netJob.get(), &NetJob::failed, [weak, callbacks](const QString& reason) {
        int network_error_code = -1;
        if (auto netJob = weak.lock()) {
            if (auto* failed_action = netJob->getFailedActions().at(0); failed_action)
                network_error_code = failed_action->replyStatusCode();
        }
        callbacks.on_fail(reason, network_error_code);
    });
    return netJob;
}
}  // namespace API
