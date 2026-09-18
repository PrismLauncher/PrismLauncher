#include "FlameCheckUpdate.h"
#include <qlist.h>
#include "Application.h"
#include "FlameAPI.h"

#include <QHash>
#include <memory>

#include "Json.h"

#include "QObjectPtr.h"
#include "ResourceDownloadTask.h"

#include "minecraft/mod/tasks/GetModDependenciesTask.h"

#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlamePackIndex.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"
#include "net/RPCSink.h"
#include "tasks/Task.h"

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

    auto* netJob = new NetJob("Get latest versions", APPLICATION->network());
    connect(netJob, &Task::finished, this, &FlameCheckUpdate::collectBlockedMods);

    connect(netJob, &Task::progress, this, &FlameCheckUpdate::setProgress);
    connect(netJob, &Task::stepProgress, this, &FlameCheckUpdate::propagateStepProgress);
    connect(netJob, &Task::details, this, &FlameCheckUpdate::setDetails);
    for (auto* resource : m_resources) {
        auto project = std::make_shared<ModPlatform::IndexedPack>();
        project->addonId = resource->metadata()->project_id.toString();
        auto spec = FlameAPI::get().getVersions({ .pack = project, .mcVersions = m_gameVersions });
        auto [task, response] = Net::RPC::make<QList<ModPlatform::IndexedVersion>>(spec);

        connect(task.get(), &Task::succeeded, this, [this, resource, response] { getLatestVersionCallback(resource, response); });
        netJob->addNetAction(task);
    }
    m_task.reset(netJob);
    m_task->start();
}

void FlameCheckUpdate::getLatestVersionCallback(Resource* resource, QList<ModPlatform::IndexedVersion>* response)
{
    auto pack = std::make_shared<ModPlatform::IndexedPack>();
    // Fake pack with the necessary info to pass to the download task :)
    pack->name = resource->name();
    pack->slug = resource->metadata()->slug;
    pack->addonId = resource->metadata()->project_id;
    pack->provider = ModPlatform::ResourceProvider::FLAME;
    pack->versions = *response;
    pack->versionsLoaded = true;

    auto latestVer = FlameAPI::getLatestVersion(pack->versions, m_loadersList, resource->metadata()->loaders, !m_loadersList.isEmpty());

    setStatus(tr("Parsing the API response from CurseForge for '%1'...").arg(resource->name()));

    if (!latestVer.has_value() || !latestVer->addonId.isValid()) {
        QString reason;
        if (dynamic_cast<Mod*>(resource) != nullptr) {
            reason =
                tr("No valid version found for this resource. It's probably unavailable for the current game "
                   "version / mod loader.");
        } else {
            reason = tr("No valid version found for this resource. It's probably unavailable for the current game version.");
        }

        emit checkFailed(resource, reason);
        return;
    }

    if (latestVer->downloadUrl.isEmpty() && latestVer->fileId != resource->metadata()->file_id) {
        m_blocked[resource] = latestVer->fileId.toString();
        return;
    }

    if (!latestVer->hash.isEmpty() &&
        (resource->metadata()->hash != latestVer->hash || resource->status() == ResourceStatus::NotInstalled)) {
        auto oldVersion = resource->metadata()->version_number;
        if (oldVersion.isEmpty()) {
            if (resource->status() == ResourceStatus::NotInstalled) {
                oldVersion = tr("Not installed");
            } else {
                oldVersion = tr("Unknown");
            }
        }

        auto downloadTask = makeShared<ResourceDownloadTask>(pack, latestVer.value(), m_resourceModel, true, "update");
        m_updates.emplace_back(pack->name, resource->metadata()->hash, oldVersion, latestVer->version, latestVer->versionType,
                               FlameAPI::getModFileChangelog(latestVer->addonId.toInt(), latestVer->fileId.toInt()),
                               ModPlatform::ResourceProvider::FLAME, downloadTask, resource->enabled());
    }
    m_deps.append(std::make_shared<GetModDependenciesTask::PackDependency>(pack, latestVer.value()));
}

void FlameCheckUpdate::collectBlockedMods()
{
    QStringList addonIds;
    QHash<QString, Resource*> quickSearch;
    for (const auto& resource : m_blocked.keys()) {
        auto addonId = resource->metadata()->project_id.toString();
        addonIds.append(addonId);
        quickSearch[addonId] = resource;
    }

    Task::Ptr projTask;

    if (addonIds.isEmpty()) {
        emitSucceeded();
        return;
    }
    if (addonIds.size() == 1) {
        auto [task, response] = FlameAPI::get().getProjectTask(*addonIds.begin());
        projTask = task;
        connect(task.get(), &Task::succeeded, this, [this, response, quickSearch] {
            auto* resource = quickSearch.find(response->addonId.toString()).value();

            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(resource->name()));

            auto recoverUrl = QString("%1/download/%2").arg(response->websiteUrl, m_blocked[resource]);
            emit checkFailed(resource, tr("Resource has a new update available, but is not downloadable using CurseForge."), recoverUrl);
        });
    } else {
        auto [task, response] = FlameAPI::get().getProjectsTask(addonIds);
        projTask = task;
        connect(projTask.get(), &Task::succeeded, this, [this, response, addonIds, quickSearch] {
            for (auto pack : *response) {
                auto* resource = quickSearch.find(pack.addonId.toString()).value();

                setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(resource->name()));

                auto recoverUrl = QString("%1/download/%2").arg(pack.websiteUrl, m_blocked[resource]);
                emit checkFailed(resource, tr("Resource has a new update available, but is not downloadable using CurseForge."),
                                 recoverUrl);
            }
        });
    }

    connect(projTask.get(), &Task::finished, this, &FlameCheckUpdate::emitSucceeded);  // do not care much about error
    connect(projTask.get(), &Task::progress, this, &FlameCheckUpdate::setProgress);
    connect(projTask.get(), &Task::stepProgress, this, &FlameCheckUpdate::propagateStepProgress);
    connect(projTask.get(), &Task::details, this, &FlameCheckUpdate::setDetails);
    m_task.reset(projTask);
    m_task->start();
}
