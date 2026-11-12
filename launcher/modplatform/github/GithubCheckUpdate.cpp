// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Prism Launcher Contributors
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

#include "GithubCheckUpdate.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "Application.h"
#include "Json.h"
#include "QObjectPtr.h"
#include "ResourceDownloadTask.h"

#include "minecraft/mod/Resource.h"

#include "modplatform/ModIndex.h"
#include "modplatform/github/GithubAPI.h"

#include "net/NetJob.h"
#include "net/Request.h"
#include "tasks/Task.h"

/* Check for update:
 * - Get the latest release of the mod's GitHub repository
 * - Compare its tag with the tag stored in the mod metadata
 * - If different, there's an update, so add it to the list
 */
void GithubCheckUpdate::executeTask()
{
    setStatus(tr("Preparing resources for GitHub..."));

    auto netJob = makeShared<NetJob>("Get latest GitHub releases", APPLICATION->network());

    connect(netJob.get(), &Task::progress, this, &GithubCheckUpdate::setProgress);
    connect(netJob.get(), &Task::stepProgress, this, &GithubCheckUpdate::propagateStepProgress);
    connect(netJob.get(), &Task::details, this, &GithubCheckUpdate::setDetails);
    connect(netJob.get(), &Task::finished, this, [this] { emitSucceeded(); });

    for (auto* resource : m_resources) {
        auto meta = resource->metadata();
        if (!meta || meta->provider != ModPlatform::ResourceProvider::GITHUB) {
            continue;
        }

        const auto repo = meta->projectId.toString();
        if (repo.isEmpty()) {
            continue;
        }

        auto url = Github::apiUrl().toString() + QStringLiteral("/repos/") + repo + QStringLiteral("/releases?per_page=10");
        auto [request, response] =
            Net::Request::makeByteArray({ .url = QUrl(url), .options = Net::Request::Option::AutoRetry, .headers = GithubAPI::headers() });

        connect(request.get(), &Task::succeeded, this, [this, resource, response] { getLatestReleaseCallback(resource, response); });

        netJob->addNetAction(request);
    }

    if (netJob->size() == 0) {
        emitSucceeded();
        return;
    }

    netJob->start();
}

void GithubCheckUpdate::getLatestReleaseCallback(Resource* resource, QByteArray* response)
{
    auto pack = std::make_shared<ModPlatform::IndexedPack>();
    ModPlatform::IndexedVersion latestVer;

    auto parse = [&]() -> Result<> {
        TRY_INTO(const auto& releases, Json::requireDocument(*response, "GitHub releases")
                                           .and_then([](const auto& v) { return Json::requireArray(v); }))

        for (const auto& releaseRaw : releases) {
            auto release = releaseRaw.toObject();
            TRY_INTO(auto version, GithubAPI::get().loadIndexedPackVersion(release, ModPlatform::ResourceType::Mod))

            const auto allowed =
                m_releaseTypes.empty() || std::ranges::contains(m_releaseTypes, version.versionType);
            if (allowed) {
                latestVer = version;
                break;
            }
        }
        return {};
    };
    if (auto res = parse(); !res) {
        qWarning() << "Error while parsing GitHub release response:" << res.error();
        qWarning() << *response;
        return;
    }

    if (!latestVer.addonId.isValid()) {
        emit checkFailed(resource, tr("No release found for this resource on GitHub."));
        return;
    }

    auto meta = resource->metadata();
    if (latestVer.versionNumber == meta->versionNumber) {
        // Up to date.
        return;
    }

    // Fake pack with the necessary info to pass to the download task :)
    pack->name = resource->name();
    pack->slug = meta->slug;
    pack->addonId = meta->projectId;
    pack->provider = ModPlatform::ResourceProvider::GITHUB;

    setStatus(tr("Parsing the API response from GitHub for '%1'...").arg(resource->name()));

    auto oldVersion = meta->versionNumber;
    if (oldVersion.isEmpty()) {
        oldVersion = resource->status() == ResourceStatus::NotInstalled ? tr("Not installed") : tr("Unknown");
    }

    auto downloadTask = makeShared<ResourceDownloadTask>(pack, latestVer, m_resourceModel, true, "update");
    m_updates.emplace_back(pack->name, meta->hash, oldVersion, latestVer.version, latestVer.versionType, latestVer.changelog,
                           ModPlatform::ResourceProvider::GITHUB, downloadTask, resource->enabled());
    m_deps.append(std::make_shared<GetModDependenciesTask::PackDependency>(pack, latestVer));
}
