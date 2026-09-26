#include "ModrinthCheckUpdate.h"
#include "Application.h"
#include "ModrinthAPI.h"

#include "QObjectPtr.h"
#include "ResourceDownloadTask.h"

#include "modplatform/ModIndex.h"
#include "modplatform/helpers/HashUtils.h"

#include "tasks/ConcurrentTask.h"

ModrinthCheckUpdate::ModrinthCheckUpdate(QList<Resource*>& resources,
                                         std::vector<Version>& mcVersions,
                                         QList<ModPlatform::ModLoaderType> loadersList,
                                         ResourceFolderModel* resourceModel,
                                         std::vector<ModPlatform::IndexedVersionType> releaseTypes)
    : CheckUpdateTask(resources, mcVersions, std::move(loadersList), resourceModel, std::move(releaseTypes))
    , m_hashType(ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH).first())
{
    if (!m_loadersList.isEmpty()) {  // this is for mods so append all the other posible loaders to the initial list
        m_initialSize = m_loadersList.length();
        ModPlatform::ModLoaderTypes modLoaders;
        for (auto* m : resources) {
            modLoaders |= m->metadata()->loaders;
        }
        for (auto l : m_loadersList) {
            modLoaders &= ~static_cast<std::uint16_t>(l);
        }
        m_loadersList.append(ModPlatform::modLoaderTypesToList(modLoaders));
    }
}

bool ModrinthCheckUpdate::abort()
{
    if (m_job) {
        return m_job->abort();
    }
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
    setProgress(0, ((m_loadersList.isEmpty() ? 1 : m_loadersList.length()) * 2) + 1);

    auto hashingTask =
        makeShared<ConcurrentTask>("MakeModrinthHashesTask", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    bool startHasing = false;
    for (auto* resource : m_resources) {
        auto hash = resource->metadata()->hash;

        // Sadly the API can only handle one hash type per call, se we
        // need to generate a new hash if the current one is innadequate
        // (though it will rarely happen, if at all)
        if (resource->metadata()->hashFormat != m_hashType) {
            auto hashTask = Hashing::createHasher(resource->fileinfo().absoluteFilePath(), ModPlatform::ResourceProvider::MODRINTH);
            connect(hashTask.get(), &Hashing::Hasher::resultsReady, this,
                    [this, resource](const QString& hash) { m_mappings.insert(hash, resource); });
            connect(hashTask.get(), &Task::failed, this, [this] { emitFailed("Failed to generate hash"); });
            hashingTask->addTask(hashTask);
            startHasing = true;
        } else {
            m_mappings.insert(hash, resource);
        }
    }

    if (startHasing) {
        connect(hashingTask.get(), &Task::finished, this, &ModrinthCheckUpdate::checkNextLoader);
        m_job = hashingTask;
        hashingTask->start();
    } else {
        checkNextLoader();
    }
}

void ModrinthCheckUpdate::getUpdateModsForLoader(std::optional<ModPlatform::ModLoaderTypes> loader, bool forceModLoaderCheck)
{
    m_loaderIdx++;

    setStatus(tr("Waiting for the API response from Modrinth..."));
    setProgress(m_progress + 1, m_progressTotal);

    QStringList hashes;
    if (forceModLoaderCheck && loader.has_value()) {
        for (const auto& hash : m_mappings.keys()) {
            if ((m_mappings.value(hash)->metadata()->loaders & loader.value()) != 0) {
                hashes.append(hash);
            }
        }
    } else {
        hashes = m_mappings.keys();
    }

    if (hashes.isEmpty()) {
        checkNextLoader();
        return;
    }

    auto [job, response] = ModrinthAPI::latestVersionsTask(hashes, m_hashType, m_gameVersions, loader, m_releaseTypes);

    connect(job.get(), &Task::succeeded, this, [this, response] { checkVersionsResponse(response); });

    connect(job.get(), &Task::failed, this, &ModrinthCheckUpdate::checkNextLoader);

    m_job = job;
    job->start();
}

void ModrinthCheckUpdate::checkVersionsResponse(QHash<QString, ModPlatform::IndexedVersion>* response)
{
    setStatus(tr("Parsing the API response from Modrinth..."));
    setProgress(m_progress + 1, m_progressTotal);

    auto iter = m_mappings.begin();

    while (iter != m_mappings.end()) {
        const QString hash = iter.key();
        Resource* resource = iter.value();

        // If the returned project is empty, but we have Modrinth metadata,
        // it means this specific version is not available
        if (!response->contains(hash)) {
            qDebug() << "Mod" << m_mappings.find(hash).value()->name() << "got an empty response. Hash:" << hash;
            ++iter;
            continue;
        }
        auto version = response->value(hash);

        // Fake pack with the necessary info to pass to the download task :)
        auto pack = std::make_shared<ModPlatform::IndexedPack>();
        pack->name = resource->name();
        pack->slug = resource->metadata()->slug;
        pack->addonId = resource->metadata()->projectId;
        pack->provider = ModPlatform::ResourceProvider::MODRINTH;
        if ((version.hash != hash && version.isPreferred) || (resource->status() == ResourceStatus::NotInstalled)) {
            auto downloadTask = makeShared<ResourceDownloadTask>(pack, version, m_resourceModel, true, "update");

            QString oldVersion = resource->metadata()->versionNumber;
            if (oldVersion.isEmpty()) {
                if (resource->status() == ResourceStatus::NotInstalled) {
                    oldVersion = tr("Not installed");
                } else {
                    oldVersion = tr("Unknown");
                }
            }

            m_updates.emplace_back(pack->name, hash, oldVersion, version.versionNumber, version.versionType, version.changelog,
                                   ModPlatform::ResourceProvider::MODRINTH, downloadTask, resource->enabled());
        }
        m_deps.append(std::make_shared<GetModDependenciesTask::PackDependency>(pack, version));

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
    if (m_loaderIdx < m_loadersList.size()) {  // this are mods so check with loades
        getUpdateModsForLoader(m_loadersList.at(m_loaderIdx), m_loaderIdx > m_initialSize);
        return;
    }
    if (m_loadersList.isEmpty() && m_loaderIdx == 0) {  // this are other resources no need to check more than once with empty loader
        getUpdateModsForLoader();
        return;
    }

    for (auto* resource : m_mappings) {
        QString reason;

        if (dynamic_cast<Mod*>(resource) != nullptr) {
            reason =
                tr("No valid version found for this resource. It's probably unavailable for the current game "
                   "version / mod loader.");
        } else {
            reason = tr("No valid version found for this resource. It's probably unavailable for the current game version.");
        }

        emit checkFailed(resource, reason);
    }

    emitSucceeded();
}
