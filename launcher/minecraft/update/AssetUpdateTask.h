#pragma once
#include "net/NetJob.h"
#include "tasks/Task.h"
class MinecraftInstance;

class AssetUpdateTask : public Task {
    Q_OBJECT
   public:
    AssetUpdateTask(MinecraftInstance* inst);
    virtual ~AssetUpdateTask() = default;

    void executeTask() override;

    bool canAbort() const override;

   private slots:
    void assetIndexFinished();
    void assetIndexFailed(QString reason);
    void assetsFailed(QString reason);

   public slots:
    bool abort() override;

   private:
    MinecraftInstance* m_inst;
    NetJob::Ptr downloadJob;
};
