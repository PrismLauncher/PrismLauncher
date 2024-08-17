#pragma once

#include "tasks/Task.h"

class MinecraftInstance;

class UpdateGlobalDirectoriesTask : public Task {
   public:
    explicit UpdateGlobalDirectoriesTask(MinecraftInstance* inst, QWidget* parent = 0);
    virtual ~UpdateGlobalDirectoriesTask();

   protected:
    virtual void executeTask() override;

   protected slots:
    void notifyFailed(QString reason);

   private:
    MinecraftInstance* m_inst;
    QWidget* m_parent;
    Task::Ptr m_tasks;
};
