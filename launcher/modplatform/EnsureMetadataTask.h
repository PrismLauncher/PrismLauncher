#pragma once

#include "ModIndex.h"

#include "modplatform/helpers/HashUtils.h"

#include <QDir>
#include "tasks/ConcurrentTask.h"

class Mod;
class QDir;

class EnsureMetadataTask : public TaskV2 {
    Q_OBJECT

   public:
    EnsureMetadataTask(Mod*, QDir, ModPlatform::ResourceProvider = ModPlatform::ResourceProvider::MODRINTH);
    EnsureMetadataTask(QList<Mod*>&, QDir, ModPlatform::ResourceProvider = ModPlatform::ResourceProvider::MODRINTH);

    ~EnsureMetadataTask() = default;

    TaskV2::Ptr getHashingTask() { return m_hashing_task; }

   protected slots:
    bool doAbort() override;
    void executeTask() override;

   private:
    // FIXME: Move to their own namespace
    TaskV2::Ptr modrinthVersionsTask();
    TaskV2::Ptr modrinthProjectsTask();

    TaskV2::Ptr flameVersionsTask();
    TaskV2::Ptr flameProjectsTask();

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
    TaskV2::Ptr m_current_task;
};
