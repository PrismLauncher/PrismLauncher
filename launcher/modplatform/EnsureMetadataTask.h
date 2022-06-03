#pragma once

#include "ModIndex.h"
#include "tasks/SequentialTask.h"

class Mod;
class QDir;
class MultipleOptionsTask;

class EnsureMetadataTask : public Task {
    Q_OBJECT

   public:
    EnsureMetadataTask(Mod&, QDir&, bool try_all, ModPlatform::Provider = ModPlatform::Provider::MODRINTH);

   public slots:
    bool abort() override;
   protected slots:
    void executeTask() override;

   private:
    // FIXME: Move to their own namespace
    void modrinthEnsureMetadata(SequentialTask&, QByteArray&);
    void flameEnsureMetadata(SequentialTask&, QByteArray&);

    // Helpers
    void emitReady();
    void emitFail();

   signals:
    void metadataReady();
    void metadataFailed();

   private:
    Mod& m_mod;
    QDir& m_index_dir;
    ModPlatform::Provider m_provider;
    bool m_try_all;

    MultipleOptionsTask* m_task_handler = nullptr;
};
