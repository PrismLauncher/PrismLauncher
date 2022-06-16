#pragma once

#include "ModIndex.h"
#include "tasks/SequentialTask.h"
#include "net/NetJob.h"

class Mod;
class QDir;
class MultipleOptionsTask;

class EnsureMetadataTask : public Task {
    Q_OBJECT

   public:
    EnsureMetadataTask(Mod&, QDir, ModPlatform::Provider = ModPlatform::Provider::MODRINTH);
    EnsureMetadataTask(std::list<Mod>&, QDir, ModPlatform::Provider = ModPlatform::Provider::MODRINTH);

    ~EnsureMetadataTask() = default;

   public slots:
    bool abort() override;
   protected slots:
    void executeTask() override;

   private:
    // FIXME: Move to their own namespace
    auto modrinthVersionsTask() -> NetJob::Ptr;
    auto modrinthProjectsTask() -> NetJob::Ptr;

    auto flameVersionsTask() -> NetJob::Ptr;
    auto flameProjectsTask() -> NetJob::Ptr;

    // Helpers
    void emitReady(Mod&);
    void emitFail(Mod&);

    auto getHash(Mod&) -> QString;

   private slots:
    void modrinthCallback(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Mod&);
    void flameCallback(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Mod&);

   signals:
    void metadataReady(Mod&);
    void metadataFailed(Mod&);

   private:
    QHash<QString, Mod> m_mods;
    QDir m_index_dir;
    ModPlatform::Provider m_provider;

    QHash<QString, ModPlatform::IndexedVersion> m_temp_versions;
    NetJob* m_current_task;
};
