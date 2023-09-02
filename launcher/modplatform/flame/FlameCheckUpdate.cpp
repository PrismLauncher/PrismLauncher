#include "FlameCheckUpdate.h"
#include "FlameAPI.h"
#include "FlameModIndex.h"

#include <MurmurHash2.h>
#include <memory>

#include "Json.h"

#include "ResourceDownloadTask.h"

#include "minecraft/mod/ModFolderModel.h"

#include "net/ApiDownload.h"

static FlameAPI api;

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

    auto response = std::make_shared<QByteArray>();
    auto url = QString("https://api.curseforge.com/v1/mods/%1").arg(ver_info.addonId.toString());
    auto dl = Net::ApiDownload::makeByteArray(url, response);
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

ModPlatform::IndexedVersion getFileInfo(int addonId, int fileId)
{
    ModPlatform::IndexedVersion ver;

    QEventLoop loop;

    auto get_file_info_job = new NetJob("Flame::GetFileInfoJob", APPLICATION->network());

    auto response = std::make_shared<QByteArray>();
    auto url = QString("https://api.curseforge.com/v1/mods/%1/files/%2").arg(QString::number(addonId), QString::number(fileId));
    auto dl = Net::ApiDownload::makeByteArray(url, response);
    get_file_info_job->addNetAction(dl);

    QObject::connect(get_file_info_job, &NetJob::succeeded, [response, &ver]() {
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
            ver = FlameMod::loadIndexedPackVersion(data_obj);
        } catch (Json::JsonException& e) {
            qWarning() << e.cause();
            qDebug() << doc;
        }
    });

    QObject::connect(get_file_info_job, &NetJob::finished, [&loop, get_file_info_job] {
        get_file_info_job->deleteLater();
        loop.quit();
    });

    get_file_info_job->start();
    loop.exec();

    return ver;
}

/* Check for update:
 * - Get latest version available
 * - Compare hash of the latest version with the current hash
 * - If equal, no updates, else, there's updates, so add to the list
 * */
void FlameCheckUpdate::executeTask()
{
    setStatus(tr("Preparing mods for CurseForge..."));

    int i = 0;
    for (auto* mod : m_mods) {
        if (!mod->enabled()) {
            emit checkFailed(mod, tr("Disabled mods won't be updated, to prevent mod duplication issues!"));
            continue;
        }

        setStatus(tr("Getting API response from CurseForge for '%1'...").arg(mod->name()));
        setProgress(i++, m_mods.size());

        auto latest_ver = api.getLatestVersion({ { mod->metadata()->project_id.toString() }, m_game_versions, m_loaders });

        // Check if we were aborted while getting the latest version
        if (m_was_aborted) {
            aborted();
            return;
        }

        setStatus(tr("Parsing the API response from CurseForge for '%1'...").arg(mod->name()));

        if (!latest_ver.addonId.isValid()) {
            emit checkFailed(mod, tr("No valid version found for this mod. It's probably unavailable for the current game "
                                     "version / mod loader."));
            continue;
        }

        if (latest_ver.downloadUrl.isEmpty() && latest_ver.fileId != mod->metadata()->file_id) {
            auto pack = getProjectInfo(latest_ver);
            auto recover_url = QString("%1/download/%2").arg(pack.websiteUrl, latest_ver.fileId.toString());
            emit checkFailed(mod, tr("Mod has a new update available, but is not downloadable using CurseForge."), recover_url);

            continue;
        }

        if (!latest_ver.hash.isEmpty() && (mod->metadata()->hash != latest_ver.hash || mod->status() == ModStatus::NotInstalled)) {
            // Fake pack with the necessary info to pass to the download task :)
            auto pack = std::make_shared<ModPlatform::IndexedPack>();
            pack->name = mod->name();
            pack->slug = mod->metadata()->slug;
            pack->addonId = mod->metadata()->project_id;
            pack->websiteUrl = mod->homeurl();
            for (auto& author : mod->authors())
                pack->authors.append({ author });
            pack->description = mod->description();
            pack->provider = ModPlatform::ResourceProvider::FLAME;

            auto old_version = mod->version();
            if (old_version.isEmpty() && mod->status() != ModStatus::NotInstalled) {
                auto current_ver = getFileInfo(latest_ver.addonId.toInt(), mod->metadata()->file_id.toInt());
                old_version = current_ver.version;
            }

            auto download_task = makeShared<ResourceDownloadTask>(pack, latest_ver, m_mods_folder);
            m_updatable.emplace_back(pack->name, mod->metadata()->hash, old_version, latest_ver.version,
                                     api.getModFileChangelog(latest_ver.addonId.toInt(), latest_ver.fileId.toInt()),
                                     ModPlatform::ResourceProvider::FLAME, download_task);
        }
    }

    emitSucceeded();
}
