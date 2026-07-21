#pragma once
#include "net/NetJob.h"
#include "tasks/Task.h"
class MinecraftInstance;

class ModUpdateTask : public Task {
    Q_OBJECT

   public:
    ModUpdateTask(MinecraftInstance* inst, bool enabled);
    virtual ~ModUpdateTask() = default;

    void executeTask() override;

    bool canAbort() const override;

   public:
    static QString resourceUrl();

   public slots:
    bool abort() override;

   private:
    MinecraftInstance* m_instance;
    bool m_enabledModsOnly;
    bool m_includeDeps = true;
};
