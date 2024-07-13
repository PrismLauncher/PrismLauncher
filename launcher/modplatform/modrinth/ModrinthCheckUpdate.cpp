#include "ModrinthCheckUpdate.h"
#include "Application.h"
#include "ModrinthAPI.h"
#include "ModrinthPackIndex.h"

#include "Json.h"

#include "ResourceDownloadTask.h"

#include "modplatform/helpers/HashUtils.h"

#include "tasks/ConcurrentTask.h"

#include "tasks/Task.h"

static ModrinthAPI api;

bool ModrinthCheckUpdate::doAbort()
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
    setStatus(tr("Preparing mods for Modrinth..."));
    setProgressTotal(3);
    setProgress(1);

    // Create all hashes
    auto best_hash_type = ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH).first();

    ConcurrentTask hashing_task(this, "MakeModrinthHashesTask", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    for (auto* mod : m_mods) {
        if (!mod->enabled()) {
            emit checkFailed(mod, tr("Disabled mods won't be updated, to prevent mod duplication issues!"));
            continue;
        }

        auto hash = mod->metadata()->hash;

        // Sadly the API can only handle one hash type per call, se we
        // need to generate a new hash if the current one is innadequate
        // (though it will rarely happen, if at all)
        if (mod->metadata()->hash_format != best_hash_type) {
            auto hash_task = Hashing::createHasher(mod->fileinfo().absoluteFilePath(), ModPlatform::ResourceProvider::MODRINTH);
            connect(hash_task.get(), &Hashing::Hasher::resultsReady, [this, mod](QString hash) {
                hashes.append(hash);
                m_mappings.insert(hash, mod);
            });
            connect(hash_task.get(), &TaskV2::finished, [this](TaskV2* t) {
                if (!t->wasSuccessful())
                    emitFailed("Failed to generate hash");
            });
            hashing_task.addTask(hash_task);
        } else {
            hashes.append(hash);
            m_mappings.insert(hash, mod);
        }
    }

    connect(&hashing_task, &TaskV2::finished, this, &ModrinthCheckUpdate::hashTaskFinished);
    hashing_task.start();
}

void ModrinthCheckUpdate::hashTaskFinished()
{
    auto best_hash_type = ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH).first();
    auto response = std::make_shared<QByteArray>();
    auto job = api.latestVersions(hashes, best_hash_type, m_game_versions, m_loaders, response);

    connect(job.get(), &TaskV2::finished, this, [this, response, best_hash_type, job](TaskV2* t) {
        if (!t->wasSuccessful()) {
            emitFailed(t->failReason());
            return;
        }
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
        setProgress(3);

        try {
            for (auto hash : m_mappings.keys()) {
                auto project_obj = doc[hash].toObject();

                // If the returned project is empty, but we have Modrinth metadata,
                // it means this specific version is not available
                if (project_obj.isEmpty()) {
                    qDebug() << "Mod " << m_mappings.find(hash).value()->name() << " got an empty response.";
                    qDebug() << "Hash: " << hash;

                    emit checkFailed(
                        m_mappings.find(hash).value(),
                        tr("No valid version found for this mod. It's probably unavailable for the current game version / mod loader."));

                    continue;
                }

                // Sometimes a version may have multiple files, one with "forge" and one with "fabric",
                // so we may want to filter it
                QString loader_filter;
                if (m_loaders.has_value()) {
                    static auto flags = { ModPlatform::ModLoaderType::NeoForge, ModPlatform::ModLoaderType::Forge,
                                          ModPlatform::ModLoaderType::Fabric, ModPlatform::ModLoaderType::Quilt };
                    for (auto flag : flags) {
                        if (m_loaders.value().testFlag(flag)) {
                            loader_filter = ModPlatform::getModLoaderAsString(flag);
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
                    qCritical() << "Modrinth mod without download url!";
                    qCritical() << project_ver.fileName;

                    emit checkFailed(m_mappings.find(hash).value(), tr("Mod has an empty download URL"));

                    continue;
                }

                auto mod_iter = m_mappings.find(hash);
                if (mod_iter == m_mappings.end()) {
                    qCritical() << "Failed to remap mod from Modrinth!";
                    continue;
                }
                auto mod = *mod_iter;

                auto key = project_ver.hash;

                // Fake pack with the necessary info to pass to the download task :)
                auto pack = std::make_shared<ModPlatform::IndexedPack>();
                pack->name = mod->name();
                pack->slug = mod->metadata()->slug;
                pack->addonId = mod->metadata()->project_id;
                pack->websiteUrl = mod->homeurl();
                for (auto& author : mod->authors())
                    pack->authors.append({ author });
                pack->description = mod->description();
                pack->provider = ModPlatform::ResourceProvider::MODRINTH;
                if ((key != hash && project_ver.is_preferred) || (mod->status() == ModStatus::NotInstalled)) {
                    if (mod->version() == project_ver.version_number)
                        continue;

                    auto download_task = makeShared<ResourceDownloadTask>(pack, project_ver, m_mods_folder);

                    m_updatable.emplace_back(pack->name, hash, mod->version(), project_ver.version_number, project_ver.version_type,
                                             project_ver.changelog, ModPlatform::ResourceProvider::MODRINTH, download_task);
                }
                m_deps.append(std::make_shared<GetModDependenciesTask::PackDependency>(pack, project_ver));
            }
        } catch (Json::JsonException& e) {
            emitFailed(e.cause() + " : " + e.what());
            return;
        }
        emitSucceeded();
    });

    setStatus(tr("Waiting for the API response from Modrinth..."));
    setProgress(2);

    m_net_job = job;
    job->start();
}
