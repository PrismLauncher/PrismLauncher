#pragma once
#include "net/NetJob.h"
#include "tasks/Task.h"
class MinecraftInstance;

class AssetUpdateTask : public TaskV2 {
    Q_OBJECT
   public:
    AssetUpdateTask(MinecraftInstance* inst);
    virtual ~AssetUpdateTask() = default;

    void executeTask() override;

   private slots:
    void assetIndexFinished(TaskV2*);

   protected slots:
    bool doAbort() override;

   private:
    MinecraftInstance* m_inst;
    NetJob::Ptr downloadJob;
};
