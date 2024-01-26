#include "ModrinthCheckUpdate.h"
#include "ModrinthAPI.h"
#include "ModrinthPackIndex.h"

#include "Json.h"

#include "ResourceDownloadTask.h"

#include "modplatform/helpers/HashUtils.h"

#include "tasks/ConcurrentTask.h"

#include "minecraft/mod/ModFolderModel.h"

static ModrinthAPI api;

bool ModrinthCheckUpdate::abort()
{
    if (m_net_job)
        return m_net_job->abort();
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
    setProgress(0, 3);

    QHash<QString, Resource*> mappings;

    // Create all hashes
    QStringList hashes;
    auto best_hash_type = ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH).first();

    ConcurrentTask hashing_task(this, "MakeModrinthHashesTask", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    for (auto* resource : m_resources) {
        if (!resource->enabled()) {
            emit checkFailed(resource, tr("Disabled resources won't be updated, to prevent resource duplication issues!"));
            continue;
        }

        auto hash = resource->metadata()->hash;

        // Sadly the API can only handle one hash type per call, se we
        // need to generate a new hash if the current one is innadequate
        // (though it will rarely happen, if at all)
        if (resource->metadata()->hash_format != best_hash_type) {
            auto hash_task = Hashing::createModrinthHasher(resource->fileinfo().absoluteFilePath());
            connect(hash_task.get(), &Hashing::Hasher::resultsReady, [&hashes, &mappings, resource](QString hash) {
                hashes.append(hash);
                mappings.insert(hash, resource);
            });
            connect(hash_task.get(), &Task::failed, [this] { failed("Failed to generate hash"); });
            hashing_task.addTask(hash_task);
        } else {
            hashes.append(hash);
            mappings.insert(hash, resource);
        }
    }

    QEventLoop loop;
    connect(&hashing_task, &Task::finished, [&loop] { loop.quit(); });
    hashing_task.start();
    loop.exec();

    auto response = std::make_shared<QByteArray>();
    auto job = api.latestVersions(hashes, best_hash_type, m_game_versions, m_loaders, response);

    connect(job.get(), &Task::succeeded, this, [this, response, mappings, best_hash_type, job] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from ModrinthCheckUpdate at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            emitFailed(parse_error.errorString());
            return;
        }

        setStatus(tr("Parsing the API response from Modrinth..."));
        setProgress(2, 3);

        try {
            for (auto iter = mappings.begin(); iter != mappings.end(); iter++) {
                const QString& hash = iter.key();
                Resource* resource = iter.value();
                auto project_obj = doc[hash].toObject();

                // If the returned project is empty, but we have Modrinth metadata,
                // it means this specific version is not available
                if (project_obj.isEmpty()) {
                    qDebug() << "Resource " << resource->name() << " got an empty response.";
                    qDebug() << "Hash: " << hash;

                    QString reason;
                    if (dynamic_cast<Mod*>(resource) != nullptr)
                        reason =
                            tr("No valid version found for this resource. It's probably unavailable for the current game "
                               "version / mod loader.");
                    else
                        reason = tr("No valid version found for this resource. It's probably unavailable for the current game version.");

                    emit checkFailed(resource, reason);
                    continue;
                }

                // Sometimes a version may have multiple files, one with "forge" and one with "fabric",
                // so we may want to filter it
                QString loader_filter;
                if (m_loaders.has_value()) {
                    static auto flags = { ModPlatform::ModLoaderType::NeoForge, ModPlatform::ModLoaderType::Forge,
                                          ModPlatform::ModLoaderType::Fabric, ModPlatform::ModLoaderType::Quilt,
                                          ModPlatform::ModLoaderType::LiteLoader };
                    for (auto flag : flags) {
                        if (m_loaders.value().testFlag(flag)) {
                            loader_filter = ModPlatform::getModLoaderString(flag);
                            break;
                        }
                    }
                }

                // Currently, we rely on a couple heuristics to determine whether an update is actually available or not:
                // - The file needs to be preferred: It is either the primary file, or the one found via (explicit) usage of the
                // loader_filter
                // - The version reported by the JAR is different from the version reported by the indexed version (it's usually the case)
                // Such is the pain of having arbitrary files for a given version .-.

                auto project_ver = Modrinth::loadIndexedPackVersion(project_obj, best_hash_type, loader_filter);
                if (project_ver.downloadUrl.isEmpty()) {
                    qCritical() << "Modrinth resource without download url!";
                    qCritical() << project_ver.fileName;

                    emit checkFailed(mappings.find(hash).value(), tr("Resource has an empty download URL"));

                    continue;
                }

                auto resource_iter = mappings.find(hash);
                if (resource_iter == mappings.end()) {
                    qCritical() << "Failed to remap resource from Modrinth!";
                    continue;
                }

                // Fake pack with the necessary info to pass to the download task :)
                auto pack = std::make_shared<ModPlatform::IndexedPack>();
                pack->name = resource->name();
                pack->slug = resource->metadata()->slug;
                pack->addonId = resource->metadata()->project_id;
                pack->provider = ModPlatform::ResourceProvider::MODRINTH;
                if ((project_ver.hash != hash && project_ver.is_preferred) || (resource->status() == ResourceStatus::NOT_INSTALLED)) {
                    auto download_task = makeShared<ResourceDownloadTask>(pack, project_ver, m_resource_model);

                    QString old_version = resource->metadata()->version_number;
                    if (old_version.isEmpty()) {
                        if (resource->status() == ResourceStatus::NOT_INSTALLED)
                            old_version = tr("Not installed");
                        else
                            old_version = tr("Unknown");
                    }

                    m_updates.emplace_back(pack->name, hash, old_version, project_ver.version_number, project_ver.version_type,
                                           project_ver.changelog, ModPlatform::ResourceProvider::MODRINTH, download_task);
                }
                m_deps.append(std::make_shared<GetModDependenciesTask::PackDependency>(pack, project_ver));
            }
        } catch (Json::JsonException& e) {
            emitFailed(e.cause() + ": " + e.what());
            return;
        }
        emitSucceeded();
    });

    connect(job.get(), &Task::failed, this, &ModrinthCheckUpdate::emitFailed);

    setStatus(tr("Waiting for the API response from Modrinth..."));
    setProgress(1, 3);

    m_net_job = qSharedPointerObjectCast<NetJob, Task>(job);
    job->start();
}
