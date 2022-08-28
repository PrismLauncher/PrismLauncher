#include "EnsureMetadataTask.h"

#include <MurmurHash2.h>
#include <QDebug>

#include "Json.h"

#include "minecraft/mod/Mod.h"
#include "minecraft/mod/tasks/LocalModUpdateTask.h"

#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

#include "net/NetJob.h"

static ModPlatform::ProviderCapabilities ProviderCaps;

static ModrinthAPI modrinth_api;
static FlameAPI flame_api;

EnsureMetadataTask::EnsureMetadataTask(Mod* mod, QDir dir, ModPlatform::Provider prov)
    : Task(nullptr), m_index_dir(dir), m_provider(prov), m_hashing_task(nullptr), m_current_task(nullptr)
{
    auto hash_task = createNewHash(mod);
    if (!hash_task)
        return;
    connect(hash_task.get(), &Task::succeeded, [this, hash_task, mod] { m_mods.insert(hash_task->getResult(), mod); });
    connect(hash_task.get(), &Task::failed, [this, hash_task, mod] { emitFail(mod, "", RemoveFromList::No); });
    hash_task->start();
}

EnsureMetadataTask::EnsureMetadataTask(QList<Mod*>& mods, QDir dir, ModPlatform::Provider prov)
    : Task(nullptr), m_index_dir(dir), m_provider(prov), m_current_task(nullptr)
{
    m_hashing_task = new ConcurrentTask(this, "MakeHashesTask", 10);
    for (auto* mod : mods) {
        auto hash_task = createNewHash(mod);
        if (!hash_task)
            continue;
        connect(hash_task.get(), &Task::succeeded, [this, hash_task, mod] { m_mods.insert(hash_task->getResult(), mod); });
        connect(hash_task.get(), &Task::failed, [this, hash_task, mod] { emitFail(mod, "", RemoveFromList::No); });
        m_hashing_task->addTask(hash_task);
    }
}

Hashing::Hasher::Ptr EnsureMetadataTask::createNewHash(Mod* mod)
{
    if (!mod || !mod->valid() || mod->type() == Mod::MOD_FOLDER)
        return nullptr;

    return Hashing::createHasher(mod->fileinfo().absoluteFilePath(), m_provider);
}

QString EnsureMetadataTask::getExistingHash(Mod* mod)
{
    // Check for already computed hashes
    // (linear on the number of mods vs. linear on the size of the mod's JAR)
    auto it = m_mods.keyValueBegin();
    while (it != m_mods.keyValueEnd()) {
        if ((*it).second == mod)
            break;
        it++;
    }

    // We already have the hash computed
    if (it != m_mods.keyValueEnd()) {
        return (*it).first;
    }

    // No existing hash
    return {};
}

bool EnsureMetadataTask::abort()
{
    // Prevent sending signals to a dead object
    disconnect(this, 0, 0, 0);

    if (m_current_task)
        return m_current_task->abort();
    return true;
}

void EnsureMetadataTask::executeTask()
{
    setStatus(tr("Checking if mods have metadata..."));

    for (auto* mod : m_mods) {
        if (!mod->valid()) {
            qDebug() << "Mod" << mod->name() << "is invalid!";
            emitFail(mod);
            continue;
        }

        // They already have the right metadata :o
        if (mod->status() != ModStatus::NoMetadata && mod->metadata() && mod->metadata()->provider == m_provider) {
            qDebug() << "Mod" << mod->name() << "already has metadata!";
            emitReady(mod);
            continue;
        }

        // Folders don't have metadata
        if (mod->type() == ResourceType::FOLDER) {
            emitReady(mod);
        }
    }

    NetJob::Ptr version_task;

    switch (m_provider) {
        case (ModPlatform::Provider::MODRINTH):
            version_task = modrinthVersionsTask();
            break;
        case (ModPlatform::Provider::FLAME):
            version_task = flameVersionsTask();
            break;
    }

    auto invalidade_leftover = [this] {
        for (auto mod = m_mods.constBegin(); mod != m_mods.constEnd(); mod++)
            emitFail(mod.value(), mod.key(), RemoveFromList::No);
        m_mods.clear();

        emitSucceeded();
    };

    connect(version_task.get(), &Task::finished, this, [this, invalidade_leftover] {
        NetJob::Ptr project_task;

        switch (m_provider) {
            case (ModPlatform::Provider::MODRINTH):
                project_task = modrinthProjectsTask();
                break;
            case (ModPlatform::Provider::FLAME):
                project_task = flameProjectsTask();
                break;
        }

        if (!project_task) {
            invalidade_leftover();
            return;
        }

        connect(project_task.get(), &Task::finished, this, [=] {
            invalidade_leftover();
            project_task->deleteLater();
            m_current_task = nullptr;
        });

        m_current_task = project_task.get();
        project_task->start();
    });

    connect(version_task.get(), &Task::finished, [=] {
        version_task->deleteLater();
        m_current_task = nullptr;
    });

    if (m_mods.size() > 1)
        setStatus(tr("Requesting metadata information from %1...").arg(ProviderCaps.readableName(m_provider)));
    else if (!m_mods.empty())
        setStatus(tr("Requesting metadata information from %1 for '%2'...")
                      .arg(ProviderCaps.readableName(m_provider), m_mods.begin().value()->name()));

    m_current_task = version_task.get();
    version_task->start();
}

void EnsureMetadataTask::emitReady(Mod* m, QString key, RemoveFromList remove)
{
    if (!m) {
        qCritical() << "Tried to mark a null mod as ready.";
        if (!key.isEmpty())
            m_mods.remove(key);

        return;
    }

    qDebug() << QString("Generated metadata for %1").arg(m->name());
    emit metadataReady(m);

    if (remove == RemoveFromList::Yes) {
        if (key.isEmpty())
            key = getExistingHash(m);
        m_mods.remove(key);
    }
}

void EnsureMetadataTask::emitFail(Mod* m, QString key, RemoveFromList remove)
{
    if (!m) {
        qCritical() << "Tried to mark a null mod as failed.";
        if (!key.isEmpty())
            m_mods.remove(key);

        return;
    }

    qDebug() << QString("Failed to generate metadata for %1").arg(m->name());
    emit metadataFailed(m);

    if (remove == RemoveFromList::Yes) {
        if (key.isEmpty())
            key = getExistingHash(m);
        m_mods.remove(key);
    }
}

// Modrinth

NetJob::Ptr EnsureMetadataTask::modrinthVersionsTask()
{
    auto hash_type = ProviderCaps.hashType(ModPlatform::Provider::MODRINTH).first();

    auto* response = new QByteArray();
    auto ver_task = modrinth_api.currentVersions(m_mods.keys(), hash_type, response);

    // Prevents unfortunate timings when aborting the task
    if (!ver_task)
        return {};

    connect(ver_task.get(), &NetJob::succeeded, this, [this, response] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth::CurrentVersions at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            failed(parse_error.errorString());
            return;
        }

        try {
            auto entries = Json::requireObject(doc);
            for (auto& hash : m_mods.keys()) {
                auto mod = m_mods.find(hash).value();
                try {
                    auto entry = Json::requireObject(entries, hash);

                    setStatus(tr("Parsing API response from Modrinth for '%1'...").arg(mod->name()));
                    qDebug() << "Getting version for" << mod->name() << "from Modrinth";

                    m_temp_versions.insert(hash, Modrinth::loadIndexedPackVersion(entry));
                } catch (Json::JsonException& e) {
                    qDebug() << e.cause();
                    qDebug() << entries;

                    emitFail(mod);
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
    });

    return ver_task;
}

NetJob::Ptr EnsureMetadataTask::modrinthProjectsTask()
{
    QHash<QString, QString> addonIds;
    for (auto const& data : m_temp_versions)
        addonIds.insert(data.addonId.toString(), data.hash);

    auto response = new QByteArray();
    NetJob::Ptr proj_task;

    if (addonIds.isEmpty()) {
        qWarning() << "No addonId found!";
    } else if (addonIds.size() == 1) {
        proj_task = modrinth_api.getProject(*addonIds.keyBegin(), response);
    } else {
        proj_task = modrinth_api.getProjects(addonIds.keys(), response);
    }

    // Prevents unfortunate timings when aborting the task
    if (!proj_task)
        return {};

    connect(proj_task.get(), &NetJob::succeeded, this, [this, response, addonIds] {
        QJsonParseError parse_error{};
        auto doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth projects task at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        try {
            QJsonArray entries;
            if (addonIds.size() == 1)
                entries = { doc.object() };
            else
                entries = Json::requireArray(doc);

            for (auto entry : entries) {
                auto entry_obj = Json::requireObject(entry);

                ModPlatform::IndexedPack pack;
                Modrinth::loadIndexedPack(pack, entry_obj);

                auto hash = addonIds.find(pack.addonId.toString()).value();

                auto mod_iter = m_mods.find(hash);
                if (mod_iter == m_mods.end()) {
                    qWarning() << "Invalid project id from the API response.";
                    continue;
                }

                auto* mod = mod_iter.value();

                try {
                    setStatus(tr("Parsing API response from Modrinth for '%1'...").arg(mod->name()));

                    modrinthCallback(pack, m_temp_versions.find(hash).value(), mod);
                } catch (Json::JsonException& e) {
                    qDebug() << e.cause();
                    qDebug() << entries;

                    emitFail(mod);
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
    });

    return proj_task;
}

// Flame
NetJob::Ptr EnsureMetadataTask::flameVersionsTask()
{
    auto* response = new QByteArray();

    QList<uint> fingerprints;
    for (auto& murmur : m_mods.keys()) {
        fingerprints.push_back(murmur.toUInt());
    }

    auto ver_task = flame_api.matchFingerprints(fingerprints, response);

    connect(ver_task.get(), &Task::succeeded, this, [this, response] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth::CurrentVersions at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            failed(parse_error.errorString());
            return;
        }

        try {
            auto doc_obj = Json::requireObject(doc);
            auto data_obj = Json::requireObject(doc_obj, "data");
            auto data_arr = Json::requireArray(data_obj, "exactMatches");

            if (data_arr.isEmpty()) {
                qWarning() << "No matches found for fingerprint search!";

                return;
            }

            for (auto match : data_arr) {
                auto match_obj = Json::ensureObject(match, {});
                auto file_obj = Json::ensureObject(match_obj, "file", {});

                if (match_obj.isEmpty() || file_obj.isEmpty()) {
                    qWarning() << "Fingerprint match is empty!";

                    return;
                }

                auto fingerprint = QString::number(Json::ensureVariant(file_obj, "fileFingerprint").toUInt());
                auto mod = m_mods.find(fingerprint);
                if (mod == m_mods.end()) {
                    qWarning() << "Invalid fingerprint from the API response.";
                    continue;
                }

                setStatus(tr("Parsing API response from CurseForge for '%1'...").arg((*mod)->name()));

                m_temp_versions.insert(fingerprint, FlameMod::loadIndexedPackVersion(file_obj));
            }

        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
    });

    return ver_task;
}

NetJob::Ptr EnsureMetadataTask::flameProjectsTask()
{
    QHash<QString, QString> addonIds;
    for (auto const& hash : m_mods.keys()) {
        if (m_temp_versions.contains(hash)) {
            auto const& data = m_temp_versions.find(hash).value();

            auto id_str = data.addonId.toString();
            if (!id_str.isEmpty())
                addonIds.insert(data.addonId.toString(), hash);
        }
    }

    auto response = new QByteArray();
    NetJob::Ptr proj_task;

    if (addonIds.isEmpty()) {
        qWarning() << "No addonId found!";
    } else if (addonIds.size() == 1) {
        proj_task = flame_api.getProject(*addonIds.keyBegin(), response);
    } else {
        proj_task = flame_api.getProjects(addonIds.keys(), response);
    }

    // Prevents unfortunate timings when aborting the task
    if (!proj_task)
        return {};

    connect(proj_task.get(), &NetJob::succeeded, this, [this, response, addonIds] {
        QJsonParseError parse_error{};
        auto doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth projects task at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        try {
            QJsonArray entries;
            if (addonIds.size() == 1)
                entries = { Json::requireObject(Json::requireObject(doc), "data") };
            else
                entries = Json::requireArray(Json::requireObject(doc), "data");

            for (auto entry : entries) {
                auto entry_obj = Json::requireObject(entry);

                auto id = QString::number(Json::requireInteger(entry_obj, "id"));
                auto hash = addonIds.find(id).value();
                auto mod = m_mods.find(hash).value();

                try {
                    setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(mod->name()));

                    ModPlatform::IndexedPack pack;
                    FlameMod::loadIndexedPack(pack, entry_obj);

                    flameCallback(pack, m_temp_versions.find(hash).value(), mod);
                } catch (Json::JsonException& e) {
                    qDebug() << e.cause();
                    qDebug() << entries;

                    emitFail(mod);
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
    });

    return proj_task;
}

void EnsureMetadataTask::modrinthCallback(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Mod* mod)
{
    // Prevent file name mismatch
    ver.fileName = mod->fileinfo().fileName();
    if (ver.fileName.endsWith(".disabled"))
        ver.fileName.chop(9);

    QDir tmp_index_dir(m_index_dir);

    {
        LocalModUpdateTask update_metadata(m_index_dir, pack, ver);
        QEventLoop loop;

        QObject::connect(&update_metadata, &Task::finished, &loop, &QEventLoop::quit);

        update_metadata.start();

        if (!update_metadata.isFinished())
            loop.exec();
    }

    auto metadata = Metadata::get(tmp_index_dir, pack.slug);
    if (!metadata.isValid()) {
        qCritical() << "Failed to generate metadata at last step!";
        emitFail(mod);
        return;
    }

    mod->setMetadata(metadata);

    emitReady(mod);
}

void EnsureMetadataTask::flameCallback(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Mod* mod)
{
    try {
        // Prevent file name mismatch
        ver.fileName = mod->fileinfo().fileName();
        if (ver.fileName.endsWith(".disabled"))
            ver.fileName.chop(9);

        QDir tmp_index_dir(m_index_dir);

        {
            LocalModUpdateTask update_metadata(m_index_dir, pack, ver);
            QEventLoop loop;

            QObject::connect(&update_metadata, &Task::finished, &loop, &QEventLoop::quit);

            update_metadata.start();

            if (!update_metadata.isFinished())
                loop.exec();
        }

        auto metadata = Metadata::get(tmp_index_dir, pack.slug);
        if (!metadata.isValid()) {
            qCritical() << "Failed to generate metadata at last step!";
            emitFail(mod);
            return;
        }

        mod->setMetadata(metadata);

        emitReady(mod);
    } catch (Json::JsonException& e) {
        qDebug() << e.cause();

        emitFail(mod);
    }
}
