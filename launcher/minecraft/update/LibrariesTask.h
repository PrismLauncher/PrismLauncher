#pragma once
#include "net/NetJob.h"
#include "tasks/Task.h"
class MinecraftInstance;

class LibrariesTask : public TaskV2 {
    Q_OBJECT
   public:
    LibrariesTask(MinecraftInstance* inst);
    virtual ~LibrariesTask() = default;

   protected:
    void executeTask() override;

   protected slots:
    bool doAbort() override;

   private:
    MinecraftInstance* m_inst;
    NetJob::Ptr downloadJob;
};
