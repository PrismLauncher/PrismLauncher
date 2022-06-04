#include "FlameCheckUpdate.h"
#include "FlameAPI.h"
#include "FlameModIndex.h"

#include <MurmurHash2.h>

#include "FileSystem.h"
#include "Json.h"

#include "ModDownloadTask.h"

static FlameAPI api;
static ModPlatform::ProviderCapabilities ProviderCaps;

bool FlameCheckUpdate::abort()
{
    m_was_aborted = true;
    if (m_net_job)
        return m_net_job->abort();
    return true;
}

ModPlatform::IndexedPack getProjectInfo(ModPlatform::IndexedVersion& ver_info)
{
    ModPlatform::IndexedPack pack;

    QEventLoop loop;

    auto get_project_job = new NetJob("Flame::GetProjectJob", APPLICATION->network());

    auto response = new QByteArray();
    auto url = QString("https://api.curseforge.com/v1/mods/%1").arg(ver_info.addonId.toString());
    auto dl = Net::Download::makeByteArray(url, response);
    get_project_job->addNetAction(dl);

    QObject::connect(get_project_job, &NetJob::succeeded, [response, &pack]() {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from FlameCheckUpdate at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        try {
            auto doc_obj = Json::requireObject(doc);
            auto data_obj = Json::requireObject(doc_obj, "data");
            FlameMod::loadIndexedPack(pack, data_obj);
        } catch (Json::JsonException& e) {
            qWarning() << e.cause();
            qDebug() << doc;
        }
    });

    QObject::connect(get_project_job, &NetJob::finished, [&loop, get_project_job] {
        get_project_job->deleteLater();
        loop.quit();
    });

    get_project_job->start();
    loop.exec();

    return pack;
}

/* Check for update:
 * - Get latest version available
 * - Compare hash of the latest version with the current hash
 * - If equal, no updates, else, there's updates, so add to the list
 * */
void FlameCheckUpdate::executeTask()
{
    setStatus(tr("Preparing mods for CurseForge..."));
    setProgress(0, 5);

    QHash<int, Mod> mappings;

    // Create all hashes
    std::list<uint> murmur_hashes;

    auto best_hash_type = ProviderCaps.hashType(ModPlatform::Provider::FLAME).first();
    for (auto mod : m_mods) {
        QByteArray jar_data;

        try {
            jar_data = FS::read(mod.fileinfo().absoluteFilePath());
        } catch (FS::FileSystemException& e) {
            qCritical() << QString("Failed to open / read JAR file of %1").arg(mod.name());
            qCritical() << QString("Reason: ") << e.cause();

            failed(e.what());
            return;
        }

        QByteArray jar_data_treated;
        for (char c : jar_data) {
            // CF-specific
            if (!(c == 9 || c == 10 || c == 13 || c == 32))
                jar_data_treated.push_back(c);
        }

        auto murmur_hash = MurmurHash2(jar_data_treated, jar_data_treated.length());
        murmur_hashes.emplace_back(murmur_hash);

        mappings.insert(mod.metadata()->mod_id().toInt(), mod);
    }

    auto* response = new QByteArray();
    auto job = api.matchFingerprints(murmur_hashes, response);

    QEventLoop lock;

    connect(job.get(), &Task::succeeded, this, [this, response, &mappings] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from FlameCheckUpdate at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            failed(parse_error.errorString());
            return;
        }

        setStatus(tr("Parsing the first API response from CurseForge..."));
        setProgress(2, 5);

        try {
            auto doc_obj = Json::requireObject(doc);
            auto data_obj = Json::ensureObject(doc_obj, "data");
            auto match_arr = Json::ensureArray(data_obj, "exactMatches");
            for (auto match : match_arr) {
                auto match_obj = Json::ensureObject(match);

                ModPlatform::IndexedVersion current_ver;
                try {
                    auto file_obj = Json::requireObject(match_obj, "file");
                    current_ver = FlameMod::loadIndexedPackVersion(file_obj);
                } catch (Json::JsonException& e) {
                    qCritical() << "Error while parsing Flame indexed version";
                    qCritical() << e.what();
                    failed(tr("An error occured while parsing a CurseForge indexed version!"));
                    return;
                }

                auto mod_iter = mappings.find(current_ver.addonId.toInt());
                if (mod_iter == mappings.end()) {
                    qCritical() << "Failed to remap mod from Flame!";
                    qDebug() << match_obj;
                    continue;
                }

                auto mod = mod_iter.value();

                setStatus(tr("Waiting for the API response from CurseForge for '%1'...").arg(mod.name()));
                setProgress(3, 5);

                auto latest_ver = api.getLatestVersion({ current_ver.addonId.toString(), m_game_versions, m_loaders });

                // Check if we were aborted while getting the latest version
                if (m_was_aborted) {
                    aborted();
                    return;
                }

                setStatus(tr("Parsing the API response from CurseForge for '%1'...").arg(mod.name()));
                setProgress(4, 5);

                if (!latest_ver.addonId.isValid()) {
                    emit checkFailed(
                        mod,
                        tr("No valid version found for this mod. It's probably unavailable for the current game version / mod loader."));
                    continue;
                }

                if (latest_ver.downloadUrl.isEmpty() && latest_ver.fileId != current_ver.fileId) {
                    auto pack = getProjectInfo(latest_ver);
                    auto recover_url = QString("%1/download/%2").arg(pack.websiteUrl, latest_ver.fileId.toString());
                    emit checkFailed(mod, tr("Mod has a new update available, but is opted-out on CurseForge"), recover_url);

                    continue;
                }

                if (!latest_ver.hash.isEmpty() && (current_ver.hash != latest_ver.hash || mod.status() == ModStatus::NotInstalled)) {
                    // Fake pack with the necessary info to pass to the download task :)
                    ModPlatform::IndexedPack pack;
                    pack.name = mod.name();
                    pack.addonId = mod.metadata()->project_id;
                    pack.websiteUrl = mod.homeurl();
                    for (auto& author : mod.authors())
                        pack.authors.append({ author });
                    pack.description = mod.description();
                    pack.provider = ModPlatform::Provider::FLAME;

                    auto download_task = new ModDownloadTask(pack, latest_ver, m_mods_folder);
                    m_updatable.emplace_back(mod.name(), current_ver.hash, current_ver.version, latest_ver.version,
                                             api.getModFileChangelog(latest_ver.addonId.toInt(), latest_ver.fileId.toInt()),
                                             ModPlatform::Provider::FLAME, download_task);
                }
            }

        } catch (Json::JsonException& e) {
            failed(e.cause() + " : " + e.what());
        }
    });

    connect(job.get(), &Task::finished, &lock, &QEventLoop::quit);

    setStatus(tr("Waiting for the first API response from CurseForge..."));
    setProgress(1, 5);

    m_net_job = job.get();
    job->start();

    lock.exec();

    emitSucceeded();
}
