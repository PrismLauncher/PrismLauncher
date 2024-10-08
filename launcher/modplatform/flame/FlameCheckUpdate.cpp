#include "FlameCheckUpdate.h"
#include "Application.h"
#include "FlameAPI.h"
#include "FlameModIndex.h"

#include <MurmurHash2.h>
#include <memory>

#include "Json.h"

#include "ResourceDownloadTask.h"

#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/tasks/GetModDependenciesTask.h"

#include "net/ApiDownload.h"

static FlameAPI api;

bool FlameCheckUpdate::abort()
{
    m_was_aborted = true;
    if (m_net_job)
        return m_net_job->abort();
    return true;
}

ModPlatform::IndexedPack FlameCheckUpdate::getProjectInfo(ModPlatform::IndexedVersion& ver_info)
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

    connect(get_project_job, &NetJob::failed, this, &FlameCheckUpdate::emitFailed);
    QObject::connect(get_project_job, &NetJob::finished, [&loop, get_project_job] {
        get_project_job->deleteLater();
        loop.quit();
    });

    get_project_job->start();
    loop.exec();

    return pack;
}

ModPlatform::IndexedVersion FlameCheckUpdate::getFileInfo(int addonId, int fileId)
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
    connect(get_file_info_job, &NetJob::failed, this, &FlameCheckUpdate::emitFailed);
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
    setStatus(tr("Preparing resources for CurseForge..."));

    int i = 0;
    for (auto* resource : m_resources) {
        setStatus(tr("Getting API response from CurseForge for '%1'...").arg(resource->name()));
        setProgress(i++, m_resources.size());

        auto latest_vers = api.getLatestVersions({ { resource->metadata()->project_id.toString() }, m_game_versions });

        // Check if we were aborted while getting the latest version
        if (m_was_aborted) {
            aborted();
            return;
        }
        auto latest_ver = api.getLatestVersion(latest_vers, m_loaders_list, resource->metadata()->loaders);

        setStatus(tr("Parsing the API response from CurseForge for '%1'...").arg(resource->name()));

        if (!latest_ver.has_value() || !latest_ver->addonId.isValid()) {
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

        if (latest_ver->downloadUrl.isEmpty() && latest_ver->fileId != resource->metadata()->file_id) {
            auto pack = getProjectInfo(latest_ver.value());
            auto recover_url = QString("%1/download/%2").arg(pack.websiteUrl, latest_ver->fileId.toString());
            emit checkFailed(resource, tr("Resource has a new update available, but is not downloadable using CurseForge."), recover_url);

            continue;
        }

        // Fake pack with the necessary info to pass to the download task :)
        auto pack = std::make_shared<ModPlatform::IndexedPack>();
        pack->name = resource->name();
        pack->slug = resource->metadata()->slug;
        pack->addonId = resource->metadata()->project_id;
        pack->provider = ModPlatform::ResourceProvider::FLAME;
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
                                   api.getModFileChangelog(latest_ver->addonId.toInt(), latest_ver->fileId.toInt()),
                                   ModPlatform::ResourceProvider::FLAME, download_task, resource->enabled());
        }
        m_deps.append(std::make_shared<GetModDependenciesTask::PackDependency>(pack, latest_ver.value()));
    }

    emitSucceeded();
}
