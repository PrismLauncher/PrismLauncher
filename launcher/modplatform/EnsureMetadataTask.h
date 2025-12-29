#pragma once

#include "ModIndex.h"
#include "net/NetJob.h"

#include <functional>

#include "modplatform/helpers/HashUtils.h"

#include "minecraft/mod/Resource.h"
#include "tasks/ConcurrentTask.h"

class Mod;
class QDir;

class EnsureMetadataTask : public Task {
    Q_OBJECT

   public:
    using IndexDirResolver = std::function<QDir(Resource*)>;

    EnsureMetadataTask(Resource*, QDir, ModPlatform::ResourceProvider = ModPlatform::ResourceProvider::MODRINTH, IndexDirResolver = {});
    EnsureMetadataTask(QList<Resource*>&,
                       QDir,
                       ModPlatform::ResourceProvider = ModPlatform::ResourceProvider::MODRINTH,
                       IndexDirResolver = {});
    EnsureMetadataTask(QHash<QString, Resource*>&,
                       QDir,
                       ModPlatform::ResourceProvider = ModPlatform::ResourceProvider::MODRINTH,
                       IndexDirResolver = {});

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
    void updateMetadata(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Resource*);
    void updateMetadataCallback(ModPlatform::IndexedPack& pack, Resource* resource);

   signals:
    void metadataReady(Resource*);
    void metadataFailed(Resource*);

   private:
    QDir indexDirForResource(Resource* resource) const;

    QHash<QString, Resource*> m_resources;
    QDir m_indexDir;
    IndexDirResolver m_indexDirResolver;
    ModPlatform::ResourceProvider m_provider;

    QHash<QString, ModPlatform::IndexedVersion> m_tempVersions;
    Task::Ptr m_hashingTask;
    Task::Ptr m_currentTask;
    QHash<QString, Task::Ptr> m_updateMetadataTasks;
};
