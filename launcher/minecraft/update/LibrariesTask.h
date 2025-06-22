#pragma once
#include "net/NetJob.h"
#include "tasks/Task.h"
class MinecraftInstance;

class LibrariesTask : public Task {
    Q_OBJECT
   public:
    LibrariesTask(MinecraftInstance* inst);
    virtual ~LibrariesTask() = default;

    void executeTask() override;

    bool canAbort() const override;

   private slots:
    void jarlibFailed(QString reason);

   public slots:
    bool abort() override;

   private:
    MinecraftInstance* m_inst;
    NetJob::Ptr downloadJob;
};
