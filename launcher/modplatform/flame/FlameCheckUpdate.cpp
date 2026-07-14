#include "FlameCheckUpdate.h"
#include "Application.h"
#include "FlameAPI.h"
#include "FlameModIndex.h"

#include <QHash>
#include <memory>

#include "Json.h"

#include "QObjectPtr.h"
#include "ResourceDownloadTask.h"

#include "minecraft/mod/tasks/GetModDependenciesTask.h"

#include "modplatform/ModIndex.h"
#include "net/ApiDownload.h"
#include "net/NetJob.h"
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
        auto versionsUrlOptional = FlameAPI().getVersionsURL({ .pack = project, .mcVersions = m_gameVersions });
        if (!versionsUrlOptional.has_value()) {
            continue;
        }

        auto [task, response] = Net::ApiDownload::makeByteArray(versionsUrlOptional.value());

        connect(task.get(), &Task::succeeded, this, [this, resource, response] { getLatestVersionCallback(resource, response); });
        netJob->addNetAction(task);
    }
    m_task.reset(netJob);
    m_task->start();
}

void FlameCheckUpdate::getLatestVersionCallback(Resource* resource, QByteArray* response)
{
    QJsonParseError parseError{};
    QJsonDocument doc = QJsonDocument::fromJson(*response, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from latest mod version at" << parseError.offset
                   << "reason:" << parseError.errorString();
        qWarning() << *response;
        return;
    }

    // Fake pack with the necessary info to pass to the download task :)
    auto pack = std::make_shared<ModPlatform::IndexedPack>();
    pack->name = resource->name();
    pack->slug = resource->metadata()->slug;
    pack->addonId = resource->metadata()->project_id;
    pack->provider = ModPlatform::ResourceProvider::FLAME;
    try {
        auto obj = Json::requireObject(doc);
        auto arr = Json::requireArray(obj, "data");

        FlameMod::loadIndexedPackVersions(*pack.get(), arr);
    } catch (Json::JsonException& e) {
        qCritical() << "Failed to parse response from a version request.";
        qCritical() << e.what();
        qDebug() << doc;
    }
    auto latestVer = FlameAPI().getLatestVersion(pack->versions, m_loadersList, resource->metadata()->loaders, !m_loadersList.isEmpty());

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
        m_updates.emplace_back(pack->name, resource->metadata()->hash, oldVersion, latestVer->version, latestVer->version_type,
                               FlameAPI().getModFileChangelog(latestVer->addonId.toInt(), latestVer->fileId.toInt()),
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
    QByteArray* response = nullptr;

    if (addonIds.isEmpty()) {
        emitSucceeded();
        return;
    }
    if (addonIds.size() == 1) {
        std::tie(projTask, response) = FlameAPI().getProject(*addonIds.begin());
    } else {
        std::tie(projTask, response) = FlameAPI().getProjects(addonIds);
    }

    connect(projTask.get(), &Task::succeeded, this, [this, response, addonIds, quickSearch] {
        QJsonParseError parseError{};
        auto doc = QJsonDocument::fromJson(*response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Flame projects task at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << *response;
            return;
        }

        try {
            QJsonArray entries;
            if (addonIds.size() == 1) {
                entries = { Json::requireObject(Json::requireObject(doc), "data") };
            } else {
                entries = Json::requireArray(Json::requireObject(doc), "data");
            }

            for (auto entry : entries) {
                auto entryObj = Json::requireObject(entry);

                auto id = QString::number(Json::requireInteger(entryObj, "id"));

                auto* resource = quickSearch.find(id).value();

                ModPlatform::IndexedPack pack;
                try {
                    setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(resource->name()));

                    FlameMod::loadIndexedPack(pack, entryObj);
                    auto recoverUrl = QString("%1/download/%2").arg(pack.websiteUrl, m_blocked[resource]);
                    emit checkFailed(resource, tr("Resource has a new update available, but is not downloadable using CurseForge."),
                                     recoverUrl);
                } catch (Json::JsonException& e) {
                    qDebug() << e.cause();
                    qDebug() << entries;
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
    });

    connect(projTask.get(), &Task::finished, this, &FlameCheckUpdate::emitSucceeded);  // do not care much about error
    connect(projTask.get(), &Task::progress, this, &FlameCheckUpdate::setProgress);
    connect(projTask.get(), &Task::stepProgress, this, &FlameCheckUpdate::propagateStepProgress);
    connect(projTask.get(), &Task::details, this, &FlameCheckUpdate::setDetails);
    m_task.reset(projTask);
    m_task->start();
}
