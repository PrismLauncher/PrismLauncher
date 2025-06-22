#pragma once

#include "tasks/Task.h"

class MinecraftInstance;

class UpdateSharedDirectoriesTask : public Task {
   public:
    explicit UpdateSharedDirectoriesTask(MinecraftInstance* inst);
    virtual ~UpdateSharedDirectoriesTask = default;

   protected:
    virtual void executeTask() override;

   protected slots:
    void notifyFailed(QString reason);

   private:
    MinecraftInstance* m_inst;
    Task::Ptr m_tasks;
};
