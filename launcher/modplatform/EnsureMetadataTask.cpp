#include "EnsureMetadataTask.h"

#include <MurmurHash2.h>
#include <QDebug>

#include "Application.h"
#include "Json.h"

#include "QObjectPtr.h"
#include "minecraft/mod/tasks/LocalResourceUpdateTask.h"

#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/helpers/HashUtils.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"
#include "settings/SettingsObject.h"
#include "tasks/ConcurrentTask.h"

EnsureMetadataTask::EnsureMetadataTask(Resource* resource, const QDir& dir, ModPlatform::ResourceProvider prov)
    : m_indexDir(dir), m_provider(prov), m_hashingTask(nullptr), m_currentTask(nullptr)
{
    auto hashTask = createNewHash(resource);
    if (!hashTask) {
        return;
    }
    connect(hashTask.get(), &Hashing::Hasher::resultsReady, this,
            [this, resource](const QString& hash) { m_resources.insert(hash, resource); });
    connect(hashTask.get(), &Task::failed, this, [this, resource] { emitFail(resource, "", RemoveFromList::No); });
    m_hashingTask = hashTask;
}

EnsureMetadataTask::EnsureMetadataTask(QList<Resource*>& resources, const QDir& dir, ModPlatform::ResourceProvider prov)
    : m_indexDir(dir), m_provider(prov), m_currentTask(nullptr)
{
    auto cHashTask = makeShared<ConcurrentTask>("MakeHashesTask", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    m_hashingTask = cHashTask;
    for (auto* resource : resources) {
        auto hashTask = createNewHash(resource);
        if (!hashTask) {
            continue;
        }
        connect(hashTask.get(), &Hashing::Hasher::resultsReady, this,
                [this, resource](const QString& hash) { m_resources.insert(hash, resource); });
        connect(hashTask.get(), &Task::failed, this, [this, resource] { emitFail(resource, "", RemoveFromList::No); });
        cHashTask->addTask(hashTask);
    }
}

EnsureMetadataTask::EnsureMetadataTask(QHash<QString, Resource*>& resources, const QDir& dir, ModPlatform::ResourceProvider prov)
    : m_resources(resources), m_indexDir(dir), m_provider(prov), m_currentTask(nullptr)
{}

Hashing::Hasher::Ptr EnsureMetadataTask::createNewHash(Resource* resource)
{
    if (!resource || !resource->valid() || resource->type() == ResourceType::FOLDER) {
        return nullptr;
    }

    return Hashing::createHasher(resource->fileinfo().absoluteFilePath(), m_provider);
}

QString EnsureMetadataTask::getExistingHash(Resource* resource)
{
    // Check for already computed hashes
    // (linear on the number of mods vs. linear on the size of the mod's JAR)
    auto it = m_resources.keyValueBegin();
    while (it != m_resources.keyValueEnd()) {
        if ((*it).second == resource) {
            break;
        }
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
    QObject::disconnect(this, nullptr, nullptr, nullptr);

    if (m_currentTask) {
        return m_currentTask->abort();
    }
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

    Task::Ptr versionTask;

    switch (m_provider) {
        case (ModPlatform::ResourceProvider::MODRINTH):
            versionTask = modrinthVersionsTask();
            break;
        case (ModPlatform::ResourceProvider::FLAME):
            versionTask = flameVersionsTask();
            break;
    }

    auto invalidadeLeftover = [this] {
        for (auto resource = m_resources.constBegin(); resource != m_resources.constEnd(); resource++) {
            emitFail(resource.value(), resource.key(), RemoveFromList::No);
        }
        m_resources.clear();

        emitSucceeded();
    };

    connect(versionTask.get(), &Task::finished, this, [this, invalidadeLeftover] {
        Task::Ptr projectTask;

        switch (m_provider) {
            case (ModPlatform::ResourceProvider::MODRINTH):
                projectTask = modrinthProjectsTask();
                break;
            case (ModPlatform::ResourceProvider::FLAME):
                projectTask = flameProjectsTask();
                break;
        }

        if (!projectTask) {
            invalidadeLeftover();
            return;
        }

        connect(projectTask.get(), &Task::finished, this, [this, invalidadeLeftover, projectTask] {
            invalidadeLeftover();
            projectTask->deleteLater();
            if (m_currentTask) {
                m_currentTask.reset();
            }
        });
        connect(projectTask.get(), &Task::failed, this, &EnsureMetadataTask::emitFailed);

        m_currentTask = projectTask;
        projectTask->start();
    });

    if (m_resources.size() > 1) {
        setStatus(tr("Requesting metadata information from %1...").arg(ModPlatform::ProviderCapabilities::readableName(m_provider)));
    } else if (!m_resources.empty()) {
        setStatus(tr("Requesting metadata information from %1 for '%2'...")
                      .arg(ModPlatform::ProviderCapabilities::readableName(m_provider), m_resources.begin().value()->name()));
    }

    m_currentTask = versionTask;
    versionTask->start();
}

void EnsureMetadataTask::emitReady(Resource* resource, QString key, RemoveFromList remove)
{
    if (!resource) {
        qCritical() << "Tried to mark a null resource as ready.";
        if (!key.isEmpty()) {
            m_resources.remove(key);
        }

        return;
    }

    qDebug() << QString("Generated metadata for %1").arg(resource->name());
    emit metadataReady(resource);

    if (remove == RemoveFromList::Yes) {
        if (key.isEmpty()) {
            key = getExistingHash(resource);
        }
        m_resources.remove(key);
    }
}

void EnsureMetadataTask::emitFail(Resource* resource, QString key, RemoveFromList remove)
{
    if (!resource) {
        qCritical() << "Tried to mark a null resource as failed.";
        if (!key.isEmpty()) {
            m_resources.remove(key);
        }

        return;
    }

    qDebug() << QString("Failed to generate metadata for %1").arg(resource->name());
    emit metadataFailed(resource);

    if (remove == RemoveFromList::Yes) {
        if (key.isEmpty()) {
            key = getExistingHash(resource);
        }
        m_resources.remove(key);
    }
}

// Modrinth

Task::Ptr EnsureMetadataTask::modrinthVersionsTask()
{
    auto hashType = ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH).first();

    auto [verTask, response] = ModrinthAPI::currentVersions(m_resources.keys(), hashType);

    // Prevents unfortunate timings when aborting the task
    if (!verTask) {
        return Task::Ptr{ nullptr };
    }

    connect(verTask.get(), &Task::succeeded, this, [this, response] {
        auto obj = Json::requireObject(*response);
        if (!obj) {
            qWarning() << "Error while parsing JSON response from Modrinth::CurrentVersions:" << obj.error();
            qWarning() << *response;

            failed(obj.error());
            return;
        }

        const auto& entries = obj.value();
        for (auto& hash : m_resources.keys()) {
            auto* resource = m_resources.find(hash).value();

            auto parse = [this, &hash, &entries, resource]() -> Result<> {
                setStatus(tr("Parsing API response from Modrinth for '%1'...").arg(resource->name()));
                qDebug() << "Getting version for" << resource->name() << "from Modrinth";

                TRY_INTO(const auto& version,
                         Json::requireObject(entries, hash).and_then([](const auto& v) { return Modrinth::loadIndexedPackVersion(v); }))

                m_tempVersions.insert(hash, version);
                return {};
            };
            if (auto res = parse(); !res) {
                qDebug() << res.error();
                qDebug() << entries;

                emitFail(resource);
                continue;
            }
        }
    });

    return verTask;
}

Task::Ptr EnsureMetadataTask::modrinthProjectsTask()
{
    QHash<QString, QString> addonIds;
    for (const auto& data : m_tempVersions) {
        addonIds.insert(data.addonId.toString(), data.hash);
    }

    Task::Ptr projTask;
    QByteArray* response = nullptr;

    if (addonIds.isEmpty()) {
        qWarning() << "No addonId found!";
    } else if (addonIds.size() == 1) {
        std::tie(projTask, response) = ModrinthAPI::get().getProject(*addonIds.keyBegin());
    } else {
        std::tie(projTask, response) = ModrinthAPI::get().getProjects(addonIds.keys());
    }

    // Prevents unfortunate timings when aborting the task
    if (!projTask) {
        return Task::Ptr{ nullptr };
    }

    connect(projTask.get(), &Task::succeeded, this, [this, response, addonIds] {
        auto doc = Json::requireDocument(*response).and_then([addonIds](const auto& v) -> Result<QJsonArray> {
            if (addonIds.size() == 1) {
                return { { v.object() } };
            }
            return Json::requireArray(v);
        });
        if (!doc) {
            qWarning() << "Error while parsing JSON response from Modrinth projects task:" << doc.error();
            qWarning() << *response;
            return;
        }

        for (auto entry : doc.value()) {
            ModPlatform::IndexedPack pack;

            auto parse = [this, &entry, &pack, &addonIds]() -> Result<> {
                TRY(Json::requireObject(entry).and_then([&pack](const auto& v) { return Modrinth::loadIndexedPack(pack, v); }))

                auto hash = addonIds.find(pack.addonId.toString()).value();

                auto resourceIter = m_resources.find(hash);
                if (resourceIter == m_resources.end()) {
                    return std::unexpected{ "Invalid project id from the API response." };
                }

                auto* resource = resourceIter.value();

                setStatus(tr("Parsing API response from Modrinth for '%1'...").arg(resource->name()));

                updateMetadata(pack, m_tempVersions.find(hash).value(), resource);
                return {};
            };
            if (auto res = parse(); !res) {
                qWarning() << res.error();
                qWarning() << *doc;
                continue;
            }
        }
    });

    return projTask;
}

// Flame
Task::Ptr EnsureMetadataTask::flameVersionsTask()
{
    QList<uint> fingerprints;
    for (auto& murmur : m_resources.keys()) {
        fingerprints.push_back(murmur.toUInt());
    }

    auto [verTask, response] = FlameAPI::matchFingerprints(fingerprints);

    connect(verTask.get(), &Task::succeeded, this, [this, response] {
        auto obj = Json::requireObject(*response);
        if (!obj) {
            qWarning() << "Error while parsing JSON response from Flame::CurrentVersions:" << obj.error();
            qWarning() << *response;

            failed(obj.error());
            return;
        }

        const auto& docObj = obj.value();
        auto dataObj = Json::requireObject(docObj, "data").and_then([](const auto& v) { return Json::requireArray(v, "exactMatches"); });
        if (!dataObj) {
            qDebug() << dataObj.error();
            qDebug() << *obj;
            return;
        }

        if (dataObj->isEmpty()) {
            qWarning() << "No matches found for fingerprint search!";

            return;
        }

        for (auto match : dataObj.value()) {
            auto matchObj = match.toObject();
            auto fileObj = matchObj["file"].toObject();

            if (matchObj.isEmpty() || fileObj.isEmpty()) {
                qWarning() << "Fingerprint match is empty!";

                return;
            }

            auto fingerprint = QString::number(fileObj["fileFingerprint"].toInteger());
            auto resource = m_resources.find(fingerprint);
            if (resource == m_resources.end()) {
                qWarning() << "Invalid fingerprint from the API response.";
                continue;
            }

            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg((*resource)->name()));

            auto versionRes = FlameMod::loadIndexedPackVersion(fileObj);
            if (!versionRes) {
                qDebug() << versionRes.error();
                qDebug() << *obj;
                continue;
            }
            m_tempVersions.insert(fingerprint, versionRes.value());
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

            auto idStr = data.addonId.toString();
            if (!idStr.isEmpty()) {
                addonIds.insert(data.addonId.toString(), hash);
            }
        }
    }

    Task::Ptr projTask;
    QByteArray* response = nullptr;

    if (addonIds.isEmpty()) {
        qWarning() << "No addonId found!";
    } else if (addonIds.size() == 1) {
        std::tie(projTask, response) = FlameAPI::get().getProject(*addonIds.keyBegin());
    } else {
        std::tie(projTask, response) = FlameAPI::get().getProjects(addonIds.keys());
    }

    // Prevents unfortunate timings when aborting the task
    if (!projTask) {
        return Task::Ptr{ nullptr };
    }

    connect(projTask.get(), &Task::succeeded, this, [this, response, addonIds] {
        auto entries = Json::requireObject(*response).and_then([addonIds](const auto& v) -> Result<QJsonArray> {
            if (addonIds.size() == 1) {
                TRY_INTO(const auto& obj, Json::requireObject(v, "data", "data"))
                return { { obj } };
            }
            return Json::requireArray(v, "data");
        });
        if (!entries) {
            qWarning() << "Error while parsing JSON response from Flame projects task:" << entries.error();
            qWarning() << *response;
            return;
        }

        for (auto entry : entries.value()) {
            auto entryObj = Json::requireObject(entry);
            if (!entryObj) {
                qDebug() << entryObj.error();
                qDebug() << *entries;
                continue;
            }

            auto idRes = Json::requireInteger(entryObj.value(), "id");
            if (!idRes) {
                qDebug() << idRes.error();
                qDebug() << *entries;
                continue;
            }
            auto id = QString::number(*idRes);
            auto hash = addonIds.find(id).value();
            auto* resource = m_resources.find(hash).value();

            ModPlatform::IndexedPack pack;
            setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(resource->name()));

            auto loadRes = FlameMod::loadIndexedPack(pack, entryObj.value());
            if (!loadRes) {
                qDebug() << loadRes.error();
                qDebug() << *entries;

                emitFail(resource);
            }
            updateMetadata(pack, m_tempVersions.find(hash).value(), resource);
        }
    });

    return projTask;
}

void EnsureMetadataTask::updateMetadata(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Resource* resource)
{
    // Prevent file name mismatch
    ver.fileName = resource->fileinfo().fileName();
    if (ver.fileName.endsWith(".disabled")) {
        ver.fileName.chop(9);
    }

    auto task = makeShared<LocalResourceUpdateTask>(m_indexDir, pack, ver);

    connect(task.get(), &Task::finished, this, [this, &pack, resource] { updateMetadataCallback(pack, resource); });

    m_updateMetadataTasks[ModPlatform::ProviderCapabilities::name(pack.provider) + pack.addonId.toString()] = task;
    task->start();
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
