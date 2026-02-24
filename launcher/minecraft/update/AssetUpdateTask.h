#pragma once
#include "net/NetJob.h"
#include "tasks/Task.h"
class MinecraftInstance;

class AssetUpdateTask : public Task {
    Q_OBJECT
   public:
    AssetUpdateTask(MinecraftInstance* inst);
    ~AssetUpdateTask() override = default;

    void executeTask() override;

    bool canAbort() const override;

   public:
    static QString resourceUrl();

   private slots:
    void assetIndexFinished();
    void assetIndexFailed(const QString& reason);
    void assetsFailed(const QString& reason);

   public slots:
    bool abort() override;

   private:
    MinecraftInstance* m_inst;
    NetJob::Ptr downloadJob;
};
