#include "FlameCheckUpdate.h"
#include "Application.h"
#include "FlameAPI.h"
#include "FlameModIndex.h"

#include <QHash>
#include <memory>

#include "Json.h"

#include "QObjectPtr.h"
#include "ResourceDownloadTask.h"

#include "api/Api.h"
#include "api/structures/Provider.h"
#include "api/structures/ResourceType.h"
#include "minecraft/mod/tasks/GetModDependenciesTask.h"

#include "api/structures/Project.h"
#include "net/ApiDownload.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

static FlameAPI api;

bool FlameCheckUpdate::abort()
{
    bool result = false;
    if (m_task && m_task->canAbort()) {
        result = m_task->abort();
    }
    Task::abort();
    return result;
}

/* Check for update:
 * - Get latest version available
 * - Compare hash of the latest version with the current hash
 * - If equal, no updates, else, there's updates, so add to the list
 * */
void FlameCheckUpdate::executeTask()
{
    setStatus(tr("Preparing resources for CurseForge..."));

    auto netJob = new NetJob("Get latest versions", APPLICATION->network());
    connect(netJob, &Task::finished, this, &FlameCheckUpdate::collectBlockedMods);

    connect(netJob, &Task::progress, this, &FlameCheckUpdate::setProgress);
    connect(netJob, &Task::stepProgress, this, &FlameCheckUpdate::propagateStepProgress);
    connect(netJob, &Task::details, this, &FlameCheckUpdate::setDetails);
    for (auto* resource : m_resources) {
        auto response = std::make_shared<API::VersionSearchResponse>();
        response->projectId = resource->metadata()->project_id;
        response->resourceType = Platform::ResourceType::Mod;
        auto task = API::ProviderAPI::get(Platform::Provider::FLAME)
                        ->makeGetVersionsRequest({ { resource->metadata()->project_id.toString() }, m_game_versions }, response);

        connect(task.get(), &Task::succeeded, this, [this, resource, response] { getLatestVersionCallback(resource, response->versions); });
        netJob->addNetAction(task);
    }
    m_task.reset(netJob);
    m_task->start();
}

void FlameCheckUpdate::getLatestVersionCallback(Resource* resource, QList<Platform::Version> response)
{
    // Fake pack with the necessary info to pass to the download task :)
    auto pack = std::make_shared<Platform::Project>();
    pack->name = resource->name();
    pack->slug = resource->metadata()->slug;
    pack->projectId = resource->metadata()->project_id;
    pack->provider = Platform::Provider::FLAME;
    pack->versions = response;
    pack->versionsLoaded = true;

    auto latest_ver = api.getLatestVersion(pack->versions, m_loaders_list, resource->metadata()->loaders);

    setStatus(tr("Parsing the API response from CurseForge for '%1'...").arg(resource->name()));

    if (!latest_ver.has_value() || !latest_ver->projectId.isValid()) {
        QString reason;
        if (dynamic_cast<Mod*>(resource) != nullptr)
            reason =
                tr("No valid version found for this resource. It's probably unavailable for the current game "
                   "version / mod loader.");
        else
            reason = tr("No valid version found for this resource. It's probably unavailable for the current game version.");

        emit checkFailed(resource, reason);
        return;
    }

    if (latest_ver->downloadUrl.isEmpty() && latest_ver->fileId != resource->metadata()->file_id) {
        m_blocked[resource] = latest_ver->fileId.toString();
        return;
    }

    if (!latest_ver->hash.isEmpty() &&
        (resource->metadata()->hash != latest_ver->hash || resource->status() == ResourceStatus::NOT_INSTALLED)) {
        auto old_version = resource->metadata()->version_number;
        if (old_version.isEmpty()) {
            if (resource->status() == ResourceStatus::NOT_INSTALLED)
                old_version = tr("Not installed");
            else
                old_version = tr("Unknown");
        }

        auto download_task = makeShared<ResourceDownloadTask>(pack, latest_ver.value(), m_resource_model);
        m_updates.emplace_back(pack->name, resource->metadata()->hash, old_version, latest_ver->version, latest_ver->version_type,
                               api.getModFileChangelog(latest_ver->projectId.toInt(), latest_ver->fileId.toInt()),
                               Platform::Provider::FLAME, download_task, resource->enabled());
    }
    m_deps.append(std::make_shared<GetModDependenciesTask::PackDependency>(pack, latest_ver.value()));
}

void FlameCheckUpdate::collectBlockedMods()
{
    QStringList addonIds;
    QHash<QString, Resource*> quickSearch;
    for (auto const& resource : m_blocked.keys()) {
        auto addonId = resource->metadata()->project_id.toString();
        addonIds.append(addonId);
        quickSearch[addonId] = resource;
    }

    Net::NetRequest::Ptr projTask;
    auto response = std::make_shared<Platform::Project>();
    auto responses = std::make_shared<QList<Platform::Project::Ptr>>();

    auto api = API::ProviderAPI::get(Platform::Provider::FLAME);
    if (addonIds.isEmpty()) {
        emitSucceeded();
        return;
    } else if (addonIds.size() == 1) {
        projTask = api->makeGetProjectRequest(*addonIds.begin(), response);
    } else {
        projTask = api->makeGetProjectsRequest(addonIds, responses);
    }
    auto netJob = makeShared<NetJob>(QString("Flame::GetProjects"), APPLICATION->network());
    netJob->addNetAction(projTask);
    connect(netJob.get(), &Task::succeeded, this, [this, response, responses, addonIds, quickSearch] {
        auto update = [this, quickSearch](Platform::Project::Ptr response) {
            auto id = response->projectId.toString();
            auto resource = quickSearch.find(id).value();
            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(resource->name()));
            auto recover_url = QString("%1/download/%2").arg(response->websiteUrl, m_blocked[resource]);
            emit checkFailed(resource, tr("Resource has a new update available, but is not downloadable using CurseForge."), recover_url);
        };
        if (addonIds.size() == 1) {
            update(response);
        } else {
            for (auto respone : *responses) {
                update(response);
            }
        }
    });

    connect(netJob.get(), &Task::finished, this, &FlameCheckUpdate::emitSucceeded);  // do not care much about error
    connect(netJob.get(), &Task::progress, this, &FlameCheckUpdate::setProgress);
    connect(netJob.get(), &Task::stepProgress, this, &FlameCheckUpdate::propagateStepProgress);
    connect(netJob.get(), &Task::details, this, &FlameCheckUpdate::setDetails);
    m_task.reset(netJob);
    m_task->start();
}