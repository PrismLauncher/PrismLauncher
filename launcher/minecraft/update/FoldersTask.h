#pragma once

#include "tasks/Task.h"

class MinecraftInstance;
class FoldersTask : public Task {
    Q_OBJECT
   public:
    FoldersTask(MinecraftInstance* inst);
    virtual ~FoldersTask() = default;

    void executeTask() override;

   private:
    MinecraftInstance* m_inst;
};
