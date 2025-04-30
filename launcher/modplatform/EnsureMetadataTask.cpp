#include "EnsureMetadataTask.h"

#include <MurmurHash2.h>
#include <QDebug>
#include <QList>

#include "Application.h"
#include "Json.h"

#include "QObjectPtr.h"
#include "api/Api.h"
#include "api/structures/Arguments.h"
#include "api/structures/Project.h"
#include "api/structures/Provider.h"
#include "minecraft/mod/tasks/LocalResourceUpdateTask.h"

#include "modplatform/helpers/HashUtils.h"
#include "net/NetJob.h"
#include "net/NetRequest.h"
#include "tasks/ConcurrentTask.h"

EnsureMetadataTask::EnsureMetadataTask(Resource* resource, QDir dir, Platform::Provider prov)
    : Task(), m_indexDir(dir), m_provider(prov), m_hashingTask(nullptr), m_currentTask(nullptr)
{
    auto hashTask = createNewHash(resource);
    if (!hashTask)
        return;
    connect(hashTask.get(), &Hashing::Hasher::resultsReady, [this, resource](QString hash) { m_resources.insert(hash, resource); });
    connect(hashTask.get(), &Task::failed, [this, resource] { emitFail(resource, "", RemoveFromList::No); });
    m_hashingTask = hashTask;
}

EnsureMetadataTask::EnsureMetadataTask(QList<Resource*>& resources, QDir dir, Platform::Provider prov)
    : Task(), m_indexDir(dir), m_provider(prov), m_currentTask(nullptr)
{
    auto hashTask = makeShared<ConcurrentTask>("MakeHashesTask", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    m_hashingTask = hashTask;
    for (auto* resource : resources) {
        auto hash_task = createNewHash(resource);
        if (!hash_task)
            continue;
        connect(hash_task.get(), &Hashing::Hasher::resultsReady, [this, resource](QString hash) { m_resources.insert(hash, resource); });
        connect(hash_task.get(), &Task::failed, [this, resource] { emitFail(resource, "", RemoveFromList::No); });
        hashTask->addTask(hash_task);
    }
}

EnsureMetadataTask::EnsureMetadataTask(QHash<QString, Resource*>& resources, QDir dir, Platform::Provider prov)
    : Task(), m_resources(resources), m_indexDir(dir), m_provider(prov), m_currentTask(nullptr)
{}

Hashing::Hasher::Ptr EnsureMetadataTask::createNewHash(Resource* resource)
{
    if (!resource || !resource->valid() || resource->type() == ResourceType::FOLDER)
        return nullptr;

    return Hashing::createHasher(resource->fileinfo().absoluteFilePath(), m_provider);
}

QString EnsureMetadataTask::getExistingHash(Resource* resource)
{
    // Check for already computed hashes
    // (linear on the number of mods vs. linear on the size of the mod's JAR)
    auto it = m_resources.keyValueBegin();
    while (it != m_resources.keyValueEnd()) {
        if ((*it).second == resource)
            break;
        it++;
    }

    // We already have the hash computed
    if (it != m_resources.keyValueEnd()) {
        return (*it).first;
    }

    // No existing hash
    return {};
}

bool EnsureMetadataTask::abort()
{
    // Prevent sending signals to a dead object
    disconnect(this, 0, 0, 0);

    if (m_currentTask)
        return m_currentTask->abort();
    return true;
}

void EnsureMetadataTask::executeTask()
{
    setStatus(tr("Checking if resources have metadata..."));

    for (auto* resource : m_resources) {
        if (!resource->valid()) {
            qDebug() << "Resource" << resource->name() << "is invalid!";
            emitFail(resource);
            continue;
        }

        // They already have the right metadata :o
        if (resource->status() != ResourceStatus::NO_METADATA && resource->metadata() && resource->metadata()->provider == m_provider) {
            qDebug() << "Resource" << resource->name() << "already has metadata!";
            emitReady(resource);
            continue;
        }

        // Folders don't have metadata
        if (resource->type() == ResourceType::FOLDER) {
            emitReady(resource);
        }
    }

    Task::Ptr version_task;

    switch (m_provider) {
        case (Platform::Provider::MODRINTH):
            version_task = modrinthVersionsTask();
            break;
        case (Platform::Provider::FLAME):
            version_task = flameVersionsTask();
            break;
    }

    auto invalidade_leftover = [this] {
        for (auto resource = m_resources.constBegin(); resource != m_resources.constEnd(); resource++)
            emitFail(resource.value(), resource.key(), RemoveFromList::No);
        m_resources.clear();

        emitSucceeded();
    };

    connect(version_task.get(), &Task::finished, this, [this, invalidade_leftover] {
        Task::Ptr project_task;

        switch (m_provider) {
            case (Platform::Provider::MODRINTH):
                project_task = modrinthProjectsTask();
                break;
            case (Platform::Provider::FLAME):
                project_task = flameProjectsTask();
                break;
        }

        if (!project_task) {
            invalidade_leftover();
            return;
        }

        connect(project_task.get(), &Task::finished, this, [this, invalidade_leftover, project_task] {
            invalidade_leftover();
            project_task->deleteLater();
            if (m_currentTask)
                m_currentTask.reset();
        });
        connect(project_task.get(), &Task::failed, this, &EnsureMetadataTask::emitFailed);

        m_currentTask = project_task;
        project_task->start();
    });

    if (m_resources.size() > 1)
        setStatus(tr("Requesting metadata information from %1...").arg(Platform::ProviderUtils::readableName(m_provider)));
    else if (!m_resources.empty())
        setStatus(tr("Requesting metadata information from %1 for '%2'...")
                      .arg(Platform::ProviderUtils::readableName(m_provider), m_resources.begin().value()->name()));

    m_currentTask = version_task;
    version_task->start();
}

void EnsureMetadataTask::emitReady(Resource* resource, QString key, RemoveFromList remove)
{
    if (!resource) {
        qCritical() << "Tried to mark a null resource as ready.";
        if (!key.isEmpty())
            m_resources.remove(key);

        return;
    }

    qDebug() << QString("Generated metadata for %1").arg(resource->name());
    emit metadataReady(resource);

    if (remove == RemoveFromList::Yes) {
        if (key.isEmpty())
            key = getExistingHash(resource);
        m_resources.remove(key);
    }
}

void EnsureMetadataTask::emitFail(Resource* resource, QString key, RemoveFromList remove)
{
    if (!resource) {
        qCritical() << "Tried to mark a null resource as failed.";
        if (!key.isEmpty())
            m_resources.remove(key);

        return;
    }

    qDebug() << QString("Failed to generate metadata for %1").arg(resource->name());
    emit metadataFailed(resource);

    if (remove == RemoveFromList::Yes) {
        if (key.isEmpty())
            key = getExistingHash(resource);
        m_resources.remove(key);
    }
}

// Modrinth

Task::Ptr EnsureMetadataTask::modrinthVersionsTask()
{
    auto hashType = Platform::ProviderUtils::hashTypeAlg(Platform::Provider::MODRINTH).first();

    auto response = std::make_shared<API::MatchHashesResponse>();
    auto ver_task = API::getModrinth()->makeMatchHashesRequest({ m_resources.keys(), hashType }, response);

    // Prevents unfortunate timings when aborting the task
    if (!ver_task)
        return Task::Ptr{ nullptr };
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetHashes"), APPLICATION->network());
    netJob->addNetAction(ver_task);
    netJob->setAskRetry(false);

    connect(netJob.get(), &Task::succeeded, this, [this, response] {
        for (auto& hash : m_resources.keys()) {
            auto resource = m_resources.find(hash).value();
            if (response->contains(hash)) {
                auto entry = response->value(hash);

                setStatus(tr("Parsing API response from Modrinth for '%1'...").arg(resource->name()));
                qDebug() << "Getting version for" << resource->name() << "from Modrinth";

                m_tempVersions.insert(hash, entry);
            } else {
                emitFail(resource);
            }
        }
    });

    return netJob;
}

Task::Ptr EnsureMetadataTask::modrinthProjectsTask()
{
    QHash<QString, QString> addonIds;
    for (auto const& data : m_tempVersions)
        addonIds.insert(data.projectId.toString(), data.hashes.first().hash);

    auto response = std::make_shared<Platform::Project>();
    auto responses = std::make_shared<QList<Platform::Project::Ptr>>();
    Net::NetRequest::Ptr proj_task;

    auto api = API::getModrinth();
    if (addonIds.isEmpty()) {
        qWarning() << "No addonId found!";
    } else if (addonIds.size() == 1) {
        proj_task = api->makeGetProjectRequest(*addonIds.keyBegin(), response);
    } else {
        proj_task = api->makeGetProjectsRequest(addonIds.keys(), responses);
    }

    // Prevents unfortunate timings when aborting the task
    if (!proj_task)
        return Task::Ptr{ nullptr };

    auto netJob = makeShared<NetJob>(QString("Modrinth::GetProjects"), APPLICATION->network());
    netJob->addNetAction(proj_task);
    connect(netJob.get(), &Task::succeeded, this, [this, response, responses, addonIds] {
        auto update = [this, addonIds](Platform::Project::Ptr response) {
            auto id = response->projectId.toString();
            auto hash = addonIds.find(id).value();
            auto resource = m_resources.find(hash).value();
            setStatus(tr("Parsing API response from Modrinth for '%1'...").arg(resource->name()));
            updateMetadata(*response, m_tempVersions.find(hash).value(), resource);
        };
        if (addonIds.size() == 1) {
            update(response);
        } else {
            for (auto response : *responses) {
                update(response);
            }
        }
    });

    return netJob;
}

// Flame
Task::Ptr EnsureMetadataTask::flameVersionsTask()
{
    auto response = std::make_shared<API::MatchHashesResponse>();

    QStringList fingerprints = m_resources.keys();

    auto task = API::getFlame()->makeMatchHashesRequest({ fingerprints }, response);

    auto ver_task = makeShared<NetJob>(QString("Flame::GetHashes"), APPLICATION->network());
    ver_task->addNetAction(task);
    ver_task->setAskRetry(false);

    connect(ver_task.get(), &Task::succeeded, this, [this, response] {
        for (auto fingerprint : response->keys()) {
            auto resource = m_resources.find(fingerprint);
            if (resource == m_resources.end()) {
                qWarning() << "Invalid fingerprint from the API response.";
                continue;
            }

            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg((*resource)->name()));

            m_tempVersions.insert(fingerprint, response->value(fingerprint));
        }
    });

    return ver_task;
}

Task::Ptr EnsureMetadataTask::flameProjectsTask()
{
    QHash<QString, QString> addonIds;
    for (auto const& hash : m_resources.keys()) {
        if (m_tempVersions.contains(hash)) {
            auto data = m_tempVersions.find(hash).value();

            auto id_str = data.projectId.toString();
            if (!id_str.isEmpty())
                addonIds.insert(data.projectId.toString(), hash);
        }
    }

    auto response = std::make_shared<Platform::Project>();
    auto responses = std::make_shared<QList<Platform::Project::Ptr>>();
    Net::NetRequest::Ptr proj_task;

    auto api = API::getFlame();
    if (addonIds.isEmpty()) {
        qWarning() << "No addonId found!";
    } else if (addonIds.size() == 1) {
        proj_task = api->makeGetProjectRequest(*addonIds.keyBegin(), response);
    } else {
        proj_task = api->makeGetProjectsRequest(addonIds.keys(), responses);
    }

    // Prevents unfortunate timings when aborting the task
    if (!proj_task)
        return Task::Ptr{ nullptr };

    auto netJob = makeShared<NetJob>(QString("Flame::GetProjects"), APPLICATION->network());
    netJob->addNetAction(proj_task);
    connect(netJob.get(), &Task::succeeded, this, [this, response, responses, addonIds] {
        auto update = [this, addonIds](Platform::Project::Ptr response) {
            auto id = response->projectId.toString();
            auto hash = addonIds.find(id).value();
            auto resource = m_resources.find(hash).value();
            setStatus(tr("Parsing API response from Curseforge for '%1'...").arg(resource->name()));
            updateMetadata(*response, m_tempVersions.find(hash).value(), resource);
        };
        if (addonIds.size() == 1) {
            update(response);
        } else {
            for (auto response : *responses) {
                update(response);
            }
        }
    });

    return netJob;
}

void EnsureMetadataTask::updateMetadata(Platform::Project& pack, Platform::Version& ver, Resource* resource)
{
    try {
        // Prevent file name mismatch
        ver.fileName = resource->fileinfo().fileName();
        if (ver.fileName.endsWith(".disabled"))
            ver.fileName.chop(9);

        auto task = makeShared<LocalResourceUpdateTask>(m_indexDir, pack, ver);

        connect(task.get(), &Task::finished, this, [this, &pack, resource] { updateMetadataCallback(pack, resource); });

        m_updateMetadataTasks[Platform::ProviderUtils::name(pack.provider) + pack.projectId.toString()] = task;
        task->start();
    } catch (Json::JsonException& e) {
        qDebug() << e.cause();

        emitFail(resource);
    }
}

void EnsureMetadataTask::updateMetadataCallback(Platform::Project& pack, Resource* resource)
{
    QDir tmpIndexDir(m_indexDir);
    auto metadata = Metadata::get(tmpIndexDir, pack.slug);
    if (!metadata.isValid()) {
        qCritical() << "Failed to generate metadata at last step!";
        emitFail(resource);
        return;
    }

    resource->setMetadata(metadata);

    emitReady(resource);
}
