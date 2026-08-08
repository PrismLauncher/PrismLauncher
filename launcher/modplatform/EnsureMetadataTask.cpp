#include "EnsureMetadataTask.h"

#include <MurmurHash2.h>
#include <QDebug>

#include "Application.h"
#include "Json.h"

#include "QObjectPtr.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/tasks/LocalResourceUpdateTask.h"

#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/helpers/HashUtils.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

EnsureMetadataTask::EnsureMetadataTask(Resource* resource, QDir dir, ModPlatform::ResourceProvider prov)
    : Task(), m_indexDir(dir), m_provider(prov), m_hashingTask(nullptr), m_currentTask(nullptr)
{
    auto hashTask = createNewHash(resource);
    if (!hashTask)
        return;
    connect(hashTask.get(), &Hashing::Hasher::resultsReady, this, [this, resource](QString hash) { m_resources.insert(hash, resource); });
    connect(hashTask.get(), &Task::failed, this, [this, resource] { emitFail(resource, "", RemoveFromList::No); });
    m_hashingTask = hashTask;
}

EnsureMetadataTask::EnsureMetadataTask(QList<Resource*>& resources, QDir dir, ModPlatform::ResourceProvider prov)
    : Task(), m_indexDir(dir), m_provider(prov), m_currentTask(nullptr)
{
    auto hashTask = makeShared<ConcurrentTask>("MakeHashesTask", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    m_hashingTask = hashTask;
    for (auto* resource : resources) {
        auto hash_task = createNewHash(resource);
        if (!hash_task)
            continue;
        connect(hash_task.get(), &Hashing::Hasher::resultsReady, this,
                [this, resource](QString hash) { m_resources.insert(hash, resource); });
        connect(hash_task.get(), &Task::failed, this, [this, resource] { emitFail(resource, "", RemoveFromList::No); });
        hashTask->addTask(hash_task);
    }
}

EnsureMetadataTask::EnsureMetadataTask(QHash<QString, Resource*>& resources, QDir dir, ModPlatform::ResourceProvider prov)
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
        if (resource->status() != ResourceStatus::NoMetadata && resource->metadata() && resource->metadata()->provider == m_provider) {
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
        case (ModPlatform::ResourceProvider::MODRINTH):
            version_task = modrinthVersionsTask();
            break;
        case (ModPlatform::ResourceProvider::FLAME):
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
            case (ModPlatform::ResourceProvider::MODRINTH):
                project_task = modrinthProjectsTask();
                break;
            case (ModPlatform::ResourceProvider::FLAME):
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
        setStatus(tr("Requesting metadata information from %1...").arg(ModPlatform::ProviderCapabilities::readableName(m_provider)));
    else if (!m_resources.empty())
        setStatus(tr("Requesting metadata information from %1 for '%2'...")
                      .arg(ModPlatform::ProviderCapabilities::readableName(m_provider), m_resources.begin().value()->name()));

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
    auto hashType = ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH).first();

    auto [verTask, result] = ModrinthAPI::get().currentVersions(m_resources.keys(), hashType).make();

    // Prevents unfortunate timings when aborting the task
    if (!verTask) {
        return Task::Ptr{ nullptr };
    }

    connect(verTask.get(), &Task::succeeded, this, [this, result] {
        for (auto& hash : m_resources.keys()) {
            auto* resource = m_resources.find(hash).value();
            auto it = result->find(hash);
            if (it == result->end()) {
                qDebug() << "Version not found for hash" << hash << "from Modrinth";
                emitFail(resource);
                continue;
            }

            setStatus(tr("Parsing API response from Modrinth for '%1'...").arg(resource->name()));
            qDebug() << "Getting version for" << resource->name() << "from Modrinth";

            m_tempVersions.insert(hash, *it);
        }
    });

    return verTask;
}

Task::Ptr EnsureMetadataTask::modrinthProjectsTask()
{
    QHash<QString, QString> addonIds;
    for (const auto& data : m_tempVersions)
        addonIds.insert(data.addonId.toString(), data.hash);

    if (addonIds.isEmpty()) {
        qWarning() << "No addonId found!";
        return Task::Ptr{ nullptr };
    }

    if (addonIds.size() == 1) {
        auto [projTask, result] = ModrinthAPI::get().getProject(*addonIds.keyBegin()).make();

        // Prevents unfortunate timings when aborting the task
        if (!projTask) {
            return Task::Ptr{ nullptr };
        }

        connect(projTask.get(), &Task::succeeded, this, [this, result, addonIds] {
            auto pack = *result;

            auto hash = addonIds.find(pack->addonId.toString()).value();

            auto resourceIter = m_resources.find(hash);
            if (resourceIter == m_resources.end()) {
                qWarning() << "Invalid project id from the API response.";
                return;
            }

            auto* resource = resourceIter.value();

            setStatus(tr("Parsing API response from Modrinth for '%1'...").arg(resource->name()));

            updateMetadata(*pack, m_tempVersions.find(hash).value(), resource);
        });

        return projTask;
    }

    auto [proj_task, result] = ModrinthAPI::get().getProjects(addonIds.keys()).make();

    // Prevents unfortunate timings when aborting the task
    if (!proj_task)
        return Task::Ptr{ nullptr };

    connect(proj_task.get(), &Task::succeeded, this, [this, result, addonIds] {
        for (auto pack : *result) {
            auto hash = addonIds.find(pack->addonId.toString()).value();

            auto resource_iter = m_resources.find(hash);
            if (resource_iter == m_resources.end()) {
                qWarning() << "Invalid project id from the API response.";
                continue;
            }

            auto* resource = resource_iter.value();

            setStatus(tr("Parsing API response from Modrinth for '%1'...").arg(resource->name()));

            updateMetadata(*pack, m_tempVersions.find(hash).value(), resource);
        }
    });

    return proj_task;
}

// Flame
Task::Ptr EnsureMetadataTask::flameVersionsTask()
{
    QList<uint> fingerprints;
    for (auto& murmur : m_resources.keys()) {
        fingerprints.push_back(murmur.toUInt());
    }

    auto [verTask, result] = FlameAPI::matchFingerprints(fingerprints).make();

    connect(verTask.get(), &Task::succeeded, this, [this, result] {
        auto matches = *result;

        if (matches.isEmpty()) {
            qWarning() << "No matches found for fingerprint search!";

            return;
        }

        for (const auto& match : matches) {
            auto fingerprint = QString::number(match.fileFingerprint);
            auto resource = m_resources.find(fingerprint);
            if (resource == m_resources.end()) {
                qWarning() << "Invalid fingerprint from the API response.";
                continue;
            }

            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg((*resource)->name()));

            // Create a minimal QJsonObject for the file to pass to loadIndexedPackVersion
            QJsonObject fileObj;
            fileObj["modId"] = match.modId;
            fileObj["id"] = match.fileId;
            fileObj["isAvailable"] = match.isAvailable;
            fileObj["fileFingerprint"] = match.fileFingerprint;
            // Note: other fields needed by loadIndexedPackVersion will need to come from elsewhere
            // For now, we just store the fingerprint match data
            m_tempVersions.insert(fingerprint, FlameMod::loadIndexedPackVersion(fileObj));
        }
    });

    return verTask;
}

Task::Ptr EnsureMetadataTask::flameProjectsTask()
{
    QHash<QString, QString> addonIds;
    for (const auto& hash : m_resources.keys()) {
        if (m_tempVersions.contains(hash)) {
            auto data = m_tempVersions.find(hash).value();

            auto id_str = data.addonId.toString();
            if (!id_str.isEmpty())
                addonIds.insert(data.addonId.toString(), hash);
        }
    }

    if (addonIds.isEmpty()) {
        qWarning() << "No addonId found!";
        return Task::Ptr{ nullptr };
    }

    if (addonIds.size() == 1) {
        auto [proj_task, result] = FlameAPI::get().getProject(*addonIds.keyBegin()).make();

        // Prevents unfortunate timings when aborting the task
        if (!proj_task)
            return Task::Ptr{ nullptr };

        connect(proj_task.get(), &Task::succeeded, this, [this, result, addonIds] {
            auto pack = *result;

            auto hash = addonIds.find(pack->addonId.toString()).value();
            auto resource_iter = m_resources.find(hash);
            if (resource_iter == m_resources.end()) {
                qWarning() << "Invalid project id from the API response.";
                return;
            }
            auto* resource = resource_iter.value();

            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(resource->name()));

            updateMetadata(*pack, m_tempVersions.find(hash).value(), resource);
        });

        return proj_task;
    }

    auto [proj_task, result] = FlameAPI::get().getProjects(addonIds.keys()).make();

    // Prevents unfortunate timings when aborting the task
    if (!proj_task)
        return Task::Ptr{ nullptr };

    connect(proj_task.get(), &Task::succeeded, this, [this, result, addonIds] {
        for (auto pack : *result) {
            auto id = pack->addonId.toString();
            auto hash = addonIds.find(id).value();
            auto resource = m_resources.find(hash).value();

            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(resource->name()));

            updateMetadata(*pack, m_tempVersions.find(hash).value(), resource);
        }
    });

    return proj_task;
}

void EnsureMetadataTask::updateMetadata(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Resource* resource)
{
    try {
        // Prevent file name mismatch
        ver.fileName = resource->fileinfo().fileName();
        if (ver.fileName.endsWith(".disabled"))
            ver.fileName.chop(9);

        auto task = makeShared<LocalResourceUpdateTask>(m_indexDir, pack, ver);

        connect(task.get(), &Task::finished, this, [this, &pack, resource] { updateMetadataCallback(pack, resource); });

        m_updateMetadataTasks[ModPlatform::ProviderCapabilities::name(pack.provider) + pack.addonId.toString()] = task;
        task->start();
    } catch (Json::JsonException& e) {
        qDebug() << e.cause();

        emitFail(resource);
    }
}

void EnsureMetadataTask::updateMetadataCallback(ModPlatform::IndexedPack& pack, Resource* resource)
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
