#pragma once

#include "ModIndex.h"

#include "modplatform/helpers/HashUtils.h"

#include "tasks/ConcurrentTask.h"

#include <QDir>

class Mod;

class EnsureMetadataTask : public Task {
    Q_OBJECT

   public:
    EnsureMetadataTask(Mod*, QDir, ModPlatform::ResourceProvider = ModPlatform::ResourceProvider::MODRINTH);
    EnsureMetadataTask(QList<Mod*>&, QDir, ModPlatform::ResourceProvider = ModPlatform::ResourceProvider::MODRINTH);
    EnsureMetadataTask(QHash<QString, Mod*>&, QDir, ModPlatform::ResourceProvider = ModPlatform::ResourceProvider::MODRINTH);

    ~EnsureMetadataTask() = default;

    Task::Ptr getHashingTask() { return m_hashing_task; }

   public slots:
    bool abort() override;
   protected slots:
    void executeTask() override;

   private:
    // FIXME: Move to their own namespace
    auto modrinthVersionsTask() -> Task::Ptr;
    auto modrinthProjectsTask() -> Task::Ptr;

    auto flameVersionsTask() -> Task::Ptr;
    auto flameProjectsTask() -> Task::Ptr;

    // Helpers
    enum class RemoveFromList { Yes, No };
    void emitReady(Mod*, QString key = {}, RemoveFromList = RemoveFromList::Yes);
    void emitFail(Mod*, QString key = {}, RemoveFromList = RemoveFromList::Yes);

    // Hashes and stuff
    auto createNewHash(Mod*) -> Hashing::Hasher::Ptr;
    auto getExistingHash(Mod*) -> QString;

   private slots:
    void modrinthCallback(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Mod*);
    void flameCallback(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Mod*);

   signals:
    void metadataReady(Mod*);
    void metadataFailed(Mod*);

   private:
    QHash<QString, Mod*> m_mods;
    QDir m_index_dir;
    ModPlatform::ResourceProvider m_provider;

    QHash<QString, ModPlatform::IndexedVersion> m_temp_versions;
    ConcurrentTask::Ptr m_hashing_task;
    Task::Ptr m_current_task;
};
