#include "ModrinthCheckUpdate.h"
#include "ModrinthAPI.h"
#include "ModrinthPackIndex.h"

#include "FileSystem.h"
#include "Json.h"

#include "ModDownloadTask.h"

static ModrinthAPI api;
static ModPlatform::ProviderCapabilities ProviderCaps;

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
    setStatus(tr("Preparing mods for Modrinth..."));
    setProgress(0, 3);

    QHash<QString, Mod> mappings;

    // Create all hashes
    QStringList hashes;
    auto best_hash_type = ProviderCaps.hashType(ModPlatform::Provider::MODRINTH).first();
    for (auto mod : m_mods) {
        auto hash = mod.metadata()->hash;

        // Sadly the API can only handle one hash type per call, se we
        // need to generate a new hash if the current one is innadequate
        // (though it will rarely happen, if at all)
        if (mod.metadata()->hash_format != best_hash_type) {
            QByteArray jar_data;

            try {
                jar_data = FS::read(mod.fileinfo().absoluteFilePath());
            } catch (FS::FileSystemException& e) {
                qCritical() << QString("Failed to open / read JAR file of %1").arg(mod.name());
                qCritical() << QString("Reason: ") << e.cause();

                failed(e.what());
                return;
            }

            hash = QString(ProviderCaps.hash(ModPlatform::Provider::MODRINTH, jar_data, best_hash_type).toHex());
        }

        hashes.append(hash);
        mappings.insert(hash, mod);
    }

    auto* response = new QByteArray();
    auto job = api.latestVersions(hashes, best_hash_type, m_game_versions, m_loaders, response);

    QEventLoop lock;

    connect(job.get(), &Task::succeeded, this, [this, response, &mappings, best_hash_type, job] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from ModrinthCheckUpdate at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            failed(parse_error.errorString());
            return;
        }

        setStatus(tr("Parsing the API response from Modrinth..."));
        setProgress(2, 3);

        try {
            for (auto hash : mappings.keys()) {
                auto project_obj = doc[hash].toObject();

                // If the returned project is empty, but we have Modrinth metadata,
                // it means this specific version is not available
                if (project_obj.isEmpty()) {
                    qDebug() << "Mod " << mappings.find(hash).value().name() << " got an empty response.";
                    qDebug() << "Hash: " << hash;

                    emit checkFailed(
                        mappings.find(hash).value(),
                        tr("No valid version found for this mod. It's probably unavailable for the current game version / mod loader."));

                    continue;
                }

                // Sometimes a version may have multiple files, one with "forge" and one with "fabric",
                // so we may want to filter it
                QString loader_filter;
                static auto flags = { ModAPI::ModLoaderType::Forge, ModAPI::ModLoaderType::Fabric, ModAPI::ModLoaderType::Quilt };
                for (auto flag : flags) {
                    if (m_loaders.testFlag(flag)) {
                        loader_filter = api.getModLoaderString(flag);
                        break;
                    }
                }

                // Currently, we rely on a couple heuristics to determine whether an update is actually available or not:
                // - The file needs to be preferred: It is either the primary file, or the one found via (explicit) usage of the loader_filter
                // - The version reported by the JAR is different from the version reported by the indexed version (it's usually the case)
                // Such is the pain of having arbitrary files for a given version .-.

                auto project_ver = Modrinth::loadIndexedPackVersion(project_obj, best_hash_type, loader_filter);
                if (project_ver.downloadUrl.isEmpty()) {
                    qCritical() << "Modrinth mod without download url!";
                    qCritical() << project_ver.fileName;

                    emit checkFailed(mappings.find(hash).value(), tr("Mod has an empty download URL"));

                    continue;
                }

                auto mod_iter = mappings.find(hash);
                if (mod_iter == mappings.end()) {
                    qCritical() << "Failed to remap mod from Modrinth!";
                    continue;
                }
                auto mod = *mod_iter;

                auto key = project_ver.hash;
                if ((key != hash && project_ver.is_preferred) || (mod.status() == ModStatus::NotInstalled)) {
                    if (mod.version() == project_ver.version_number)
                        continue;

                    // Fake pack with the necessary info to pass to the download task :)
                    ModPlatform::IndexedPack pack;
                    pack.name = mod.name();
                    pack.slug = mod.metadata()->slug;
                    pack.addonId = mod.metadata()->project_id;
                    pack.websiteUrl = mod.homeurl();
                    for (auto& author : mod.authors())
                        pack.authors.append({ author });
                    pack.description = mod.description();
                    pack.provider = ModPlatform::Provider::MODRINTH;

                    auto download_task = new ModDownloadTask(pack, project_ver, m_mods_folder);

                    m_updatable.emplace_back(mod.name(), hash, mod.version(), project_ver.version_number, project_ver.changelog,
                                             ModPlatform::Provider::MODRINTH, download_task);
                }
            }
        } catch (Json::JsonException& e) {
            failed(e.cause() + " : " + e.what());
        }
    });

    connect(job.get(), &Task::finished, &lock, &QEventLoop::quit);

    setStatus(tr("Waiting for the API response from Modrinth..."));
    setProgress(1, 3);

    m_net_job = job.get();
    job->start();

    lock.exec();

    emitSucceeded();
}
