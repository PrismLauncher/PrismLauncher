#include "EnsureMetadataTask.h"

#include <MurmurHash2.h>
#include <QDebug>

#include "FileSystem.h"
#include "Json.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/tasks/LocalModUpdateTask.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"
#include "net/NetJob.h"
#include "tasks/MultipleOptionsTask.h"

static ModPlatform::ProviderCapabilities ProviderCaps;

static ModrinthAPI modrinth_api;
static FlameAPI flame_api;

EnsureMetadataTask::EnsureMetadataTask(Mod& mod, QDir& dir, bool try_all, ModPlatform::Provider prov)
    : m_mod(mod), m_index_dir(dir), m_provider(prov), m_try_all(try_all)
{}

bool EnsureMetadataTask::abort()
{
    return m_task_handler->abort();
}

void EnsureMetadataTask::executeTask()
{
    // They already have the right metadata :o
    if (m_mod.status() != ModStatus::NoMetadata && m_mod.metadata() && m_mod.metadata()->provider == m_provider) {
        emitReady();
        return;
    }

    // Folders don't have metadata
    if (m_mod.type() == Mod::MOD_FOLDER) {
        emitReady();
        return;
    }

    setStatus(tr("Generating %1's metadata...").arg(m_mod.name()));
    qDebug() << QString("Generating %1's metadata...").arg(m_mod.name());

    QByteArray jar_data;

    try {
        jar_data = FS::read(m_mod.fileinfo().absoluteFilePath());
    } catch (FS::FileSystemException& e) {
        qCritical() << QString("Failed to open / read JAR file of %1").arg(m_mod.name());
        qCritical() << QString("Reason: ") << e.cause();

        emitFail();
        return;
    }

    auto tsk = new MultipleOptionsTask(nullptr, "GetMetadataTask");

    switch (m_provider) {
        case (ModPlatform::Provider::MODRINTH):
            modrinthEnsureMetadata(*tsk, jar_data);
            if (m_try_all)
                flameEnsureMetadata(*tsk, jar_data);

            break;
        case (ModPlatform::Provider::FLAME):
            flameEnsureMetadata(*tsk, jar_data);
            if (m_try_all)
                modrinthEnsureMetadata(*tsk, jar_data);

            break;
    }

    connect(tsk, &MultipleOptionsTask::finished, this, [tsk] { tsk->deleteLater(); });
    connect(tsk, &MultipleOptionsTask::failed, [this] {
        qCritical() << QString("Download of %1's metadata failed").arg(m_mod.name());

        emitFail();
    });
    connect(tsk, &MultipleOptionsTask::succeeded, this, &EnsureMetadataTask::emitReady);

    m_task_handler = tsk;

    tsk->start();
}

void EnsureMetadataTask::emitReady()
{
    emit metadataReady();
    emitSucceeded();
}

void EnsureMetadataTask::emitFail()
{
    qDebug() << QString("Failed to generate metadata for %1").arg(m_mod.name());
    emit metadataFailed();
    //emitFailed(tr("Failed to generate metadata for %1").arg(m_mod.name()));
    emitSucceeded();
}

void EnsureMetadataTask::modrinthEnsureMetadata(SequentialTask& tsk, QByteArray& jar_data)
{
    // Modrinth currently garantees that some hash types will always be present.
    // But let's be sure and cover all cases anyways :)
    for (auto hash_type : ProviderCaps.hashType(ModPlatform::Provider::MODRINTH)) {
        auto* response = new QByteArray();
        auto hash = QString(ProviderCaps.hash(ModPlatform::Provider::MODRINTH, jar_data, hash_type).toHex());
        auto ver_task = modrinth_api.currentVersion(hash, hash_type, response);

        // Prevents unfortunate timings when aborting the task
        if (!ver_task)
            return;

        connect(ver_task.get(), &NetJob::succeeded, this, [this, ver_task, response] {
            QJsonParseError parse_error{};
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if (parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from " << m_mod.name() << " at " << parse_error.offset
                           << " reason: " << parse_error.errorString();
                qWarning() << *response;

                ver_task->failed(parse_error.errorString());
                return;
            }

            auto doc_obj = Json::requireObject(doc);
            auto ver = Modrinth::loadIndexedPackVersion(doc_obj, {}, m_mod.fileinfo().fileName());

            // Minimal IndexedPack to create the metadata
            ModPlatform::IndexedPack pack;
            pack.name = m_mod.name();
            pack.provider = ModPlatform::Provider::MODRINTH;
            pack.addonId = ver.addonId;

            // Prevent file name mismatch
            ver.fileName = m_mod.fileinfo().fileName();

            QDir tmp_index_dir(m_index_dir);

            {
                LocalModUpdateTask update_metadata(m_index_dir, pack, ver);
                QEventLoop loop;
                QTimer timeout;

                QObject::connect(&update_metadata, &Task::finished, &loop, &QEventLoop::quit);
                QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

                update_metadata.start();
                timeout.start(100);

                loop.exec();
            }

            auto mod_name = m_mod.name();
            auto meta = new Metadata::ModStruct(Metadata::get(tmp_index_dir, mod_name));
            m_mod.setMetadata(meta);
        });

        tsk.addTask(ver_task);
    }
}

void EnsureMetadataTask::flameEnsureMetadata(SequentialTask& tsk, QByteArray& jar_data)
{
    QByteArray jar_data_treated;
    for (char c : jar_data) {
        // CF-specific
        if (!(c == 9 || c == 10 || c == 13 || c == 32))
            jar_data_treated.push_back(c);
    }

    auto* response = new QByteArray();

    std::list<uint> fingerprints;
    auto murmur = MurmurHash2(jar_data_treated, jar_data_treated.length());
    fingerprints.push_back(murmur);

    auto ver_task = flame_api.matchFingerprints(fingerprints, response);

    connect(ver_task.get(), &Task::succeeded, this, [this, ver_task, response] {
        QDir tmp_index_dir(m_index_dir);

        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << m_mod.name() << " at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            ver_task->failed(parse_error.errorString());
            return;
        }

        try {
            auto doc_obj = Json::requireObject(doc);
            auto data_obj = Json::ensureObject(doc_obj, "data");
            auto match_obj = Json::ensureObject(Json::ensureArray(data_obj, "exactMatches")[0], {});
            if (match_obj.isEmpty()) {
                qCritical() << "Fingerprint match is empty!";

                ver_task->failed(parse_error.errorString());
                return;
            }

            auto file_obj = Json::ensureObject(match_obj, "file");

            ModPlatform::IndexedPack pack;
            pack.name = m_mod.name();
            pack.provider = ModPlatform::Provider::FLAME;
            pack.addonId = Json::requireInteger(file_obj, "modId");

            ModPlatform::IndexedVersion ver = FlameMod::loadIndexedPackVersion(file_obj);

            // Prevent file name mismatch
            ver.fileName = m_mod.fileinfo().fileName();

            {
                LocalModUpdateTask update_metadata(m_index_dir, pack, ver);
                QEventLoop loop;
                QTimer timeout;

                QObject::connect(&update_metadata, &Task::finished, &loop, &QEventLoop::quit);
                QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

                update_metadata.start();
                timeout.start(100);

                loop.exec();
            }

            auto mod_name = m_mod.name();
            auto meta = new Metadata::ModStruct(Metadata::get(tmp_index_dir, mod_name));
            m_mod.setMetadata(meta);

        } catch (Json::JsonException& e) {
            emitFailed(e.cause() + " : " + e.what());
        }
    });

    tsk.addTask(ver_task);
}
