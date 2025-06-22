#pragma once

#include "tasks/Task.h"

class MinecraftInstance;

class UpdateSharedDirectoriesTask : public Task {
   public:
    explicit UpdateSharedDirectoriesTask(MinecraftInstance* inst, QWidget* parent = 0);
    virtual ~UpdateSharedDirectoriesTask();

   protected:
    virtual void executeTask() override;

   protected slots:
    void notifyFailed(QString reason);

   private:
    MinecraftInstance* m_inst;
    QWidget* m_parent;
    Task::Ptr m_tasks;
};
