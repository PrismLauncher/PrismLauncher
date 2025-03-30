#pragma once

#include "ModIndex.h"
#include "net/NetJob.h"

#include "modplatform/helpers/HashUtils.h"

#include "minecraft/mod/Resource.h"
#include "tasks/ConcurrentTask.h"

class Mod;
class QDir;

class EnsureMetadataTask : public Task {
    Q_OBJECT

   public:
    EnsureMetadataTask(Resource*, QDir, Platform::Provider = Platform::Provider::MODRINTH);
    EnsureMetadataTask(QList<Resource*>&, QDir, Platform::Provider = Platform::Provider::MODRINTH);
    EnsureMetadataTask(QHash<QString, Resource*>&, QDir, Platform::Provider = Platform::Provider::MODRINTH);

    ~EnsureMetadataTask() = default;

    Task::Ptr getHashingTask() { return m_hashingTask; }

   public slots:
    bool abort() override;
   protected slots:
    void executeTask() override;

   private:
    // FIXME: Move to their own namespace
    Task::Ptr modrinthVersionsTask();
    Task::Ptr modrinthProjectsTask();

    Task::Ptr flameVersionsTask();
    Task::Ptr flameProjectsTask();

    // Helpers
    enum class RemoveFromList { Yes, No };
    void emitReady(Resource*, QString key = {}, RemoveFromList = RemoveFromList::Yes);
    void emitFail(Resource*, QString key = {}, RemoveFromList = RemoveFromList::Yes);

    // Hashes and stuff
    Hashing::Hasher::Ptr createNewHash(Resource*);
    QString getExistingHash(Resource*);

   private slots:
    void updateMetadata(ModPlatform::IndexedPack& pack, Platform::Version& ver, Resource*);
    void updateMetadataCallback(ModPlatform::IndexedPack& pack, Resource* resource);

   signals:
    void metadataReady(Resource*);
    void metadataFailed(Resource*);

   private:
    QHash<QString, Resource*> m_resources;
    QDir m_indexDir;
    Platform::Provider m_provider;

    QHash<QString, Platform::Version> m_tempVersions;
    Task::Ptr m_hashingTask;
    Task::Ptr m_currentTask;
    QHash<QString, Task::Ptr> m_updateMetadataTasks;
};
