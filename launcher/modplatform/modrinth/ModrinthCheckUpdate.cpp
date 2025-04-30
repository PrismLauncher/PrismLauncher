#include "ModrinthCheckUpdate.h"
#include "Application.h"

#include "QObjectPtr.h"
#include "ResourceDownloadTask.h"

#include "api/structures/Arguments.h"
#include "api/structures/Project.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/helpers/HashUtils.h"

#include "tasks/ConcurrentTask.h"

static ResourceAPI api = ResourceAPI(Platform::Provider::MODRINTH);

bool ModrinthCheckUpdate::abort()
{
    if (m_job)
        return m_job->abort();
    return true;
}

/* Check for update:
 * - Get latest version available
 * - Compare hash of the latest version with the current hash
 * - If equal, no updates, else, there's updates, so add to the list
 * */
void ModrinthCheckUpdate::executeTask()
{
    setStatus(tr("Preparing resources for Modrinth..."));
    setProgress(0, (m_loaders_list.isEmpty() ? 1 : m_loaders_list.length()) * 2 + 1);

    auto hashing_task =
        makeShared<ConcurrentTask>("MakeModrinthHashesTask", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    for (auto* resource : m_resources) {
        auto hash = resource->metadata()->hash;

        // Sadly the API can only handle one hash type per call, se we
        // need to generate a new hash if the current one is innadequate
        // (though it will rarely happen, if at all)
        if (Hashing::algorithmFromString(resource->metadata()->hash_format) != m_hash_type) {
            auto hash_task = Hashing::createHasher(resource->fileinfo().absoluteFilePath(), Platform::Provider::MODRINTH);
            connect(hash_task.get(), &Hashing::Hasher::resultsReady, [this, resource](QString hash) { m_mappings.insert(hash, resource); });
            connect(hash_task.get(), &Task::failed, [this] { failed("Failed to generate hash"); });
            hashing_task->addTask(hash_task);
        } else {
            m_mappings.insert(hash, resource);
        }
    }

    connect(hashing_task.get(), &Task::finished, this, &ModrinthCheckUpdate::checkNextLoader);
    m_job = hashing_task;
    hashing_task->start();
}

void ModrinthCheckUpdate::getUpdateModsForLoader(std::optional<Platform::ModLoaders> loader)
{
    setStatus(tr("Waiting for the API response from Modrinth..."));
    setProgress(m_progress + 1, m_progressTotal);

    auto response = std::make_shared<API::GetLatestVersionsResponse>();

    // Sometimes a version may have multiple files, one with "forge" and one with "fabric",
    // so we may want to filter it

    // Currently, we rely on a couple heuristics to determine whether an update is actually available or not:
    // - The file needs to be preferred: It is either the primary file, or the one found via (explicit) usage of the
    // loader_filter
    // - The version reported by the JAR is different from the version reported by the indexed version (it's usually the case)
    // Such is the pain of having arbitrary files for a given version .-.
    if (loader.has_value()) {
        for (auto flag : Platform::ModloaderUtils::toList(*loader)) {
            response->filter = Platform::ModloaderUtils::toString(flag);
            break;
        }
    }
    response->hashFormat = m_hash_type;
    QStringList hashes = m_mappings.keys();

    auto job = api.latestVersions({ hashes, m_hash_type, m_game_versions, loader }, response);

    connect(job.get(), &Task::succeeded, this, [this, response, loader] { checkVersionsResponse(response, loader); });

    connect(job.get(), &Task::failed, this, &ModrinthCheckUpdate::checkNextLoader);

    m_job = job;
    job->start();
}

void ModrinthCheckUpdate::checkVersionsResponse(std::shared_ptr<API::GetLatestVersionsResponse> response,
                                                std::optional<Platform::ModLoaders> loader)
{
    setStatus(tr("Parsing the API response from Modrinth..."));
    setProgress(m_progress + 1, m_progressTotal);

    auto iter = m_mappings.begin();

    while (iter != m_mappings.end()) {
        const QString hash = iter.key();
        Resource* resource = iter.value();

        // If the returned project is empty, but we have Modrinth metadata,
        // it means this specific version is not available
        if (!response->versions.contains(hash)) {
            qDebug() << "Mod " << m_mappings.find(hash).value()->name() << " got an empty response." << "Hash: " << hash;
            ++iter;
            continue;
        }
        auto project_ver = response->versions.value(hash);

        // Fake pack with the necessary info to pass to the download task :)
        auto pack = std::make_shared<Platform::Project>();
        pack->name = resource->name();
        pack->slug = resource->metadata()->slug;
        pack->projectId = resource->metadata()->project_id;
        pack->provider = Platform::Provider::MODRINTH;
        auto installed = resource->status() != ResourceStatus::NOT_INSTALLED;
        if (installed) {
            if (project_ver.hashes.empty()) {
                installed = false;
            } else {
                auto h = project_ver.hashes.first();
                installed = h.hash == hash || !project_ver.is_preferred;
            }
        }
        if (!installed) {
            auto download_task = makeShared<ResourceDownloadTask>(pack, project_ver, m_resource_model);

            QString old_version = resource->metadata()->version_number;
            if (old_version.isEmpty()) {
                if (resource->status() == ResourceStatus::NOT_INSTALLED)
                    old_version = tr("Not installed");
                else
                    old_version = tr("Unknown");
            }

            m_updates.emplace_back(pack->name, hash, old_version, project_ver.version_number, project_ver.version_type,
                                   project_ver.changelog, Platform::Provider::MODRINTH, download_task, resource->enabled());
        }
        m_deps.append(std::make_shared<GetModDependenciesTask::PackDependency>(pack, project_ver));

        iter = m_mappings.erase(iter);
    }

    checkNextLoader();
}

void ModrinthCheckUpdate::checkNextLoader()
{
    if (m_mappings.isEmpty()) {
        emitSucceeded();
        return;
    }

    if (m_loaders_list.isEmpty() && m_loader_idx == 0) {
        getUpdateModsForLoader({});
        m_loader_idx++;
        return;
    }

    if (m_loader_idx < m_loaders_list.size()) {
        getUpdateModsForLoader(m_loaders_list.at(m_loader_idx));
        m_loader_idx++;
        return;
    }

    for (auto resource : m_mappings) {
        QString reason;

        if (dynamic_cast<Mod*>(resource) != nullptr)
            reason =
                tr("No valid version found for this resource. It's probably unavailable for the current game "
                   "version / mod loader.");
        else
            reason = tr("No valid version found for this resource. It's probably unavailable for the current game version.");

        emit checkFailed(resource, reason);
    }

    emitSucceeded();
}
