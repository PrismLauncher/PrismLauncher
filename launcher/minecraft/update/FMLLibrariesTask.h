#pragma once
#include "minecraft/VersionFilterData.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class MinecraftInstance;

class FMLLibrariesTask : public TaskV2 {
    Q_OBJECT
   public:
    FMLLibrariesTask(MinecraftInstance* inst);
    virtual ~FMLLibrariesTask() = default;

   protected:
    void executeTask() override;

   private slots:
    void fmllibsFinished(TaskV2*);

   protected slots:
    bool doAbort() override;

   private:
    MinecraftInstance* m_inst;
    NetJob::Ptr downloadJob;
    QList<FMLlib> fmlLibsToProcess;
};
