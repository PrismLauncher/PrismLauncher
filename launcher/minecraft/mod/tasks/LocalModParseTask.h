#pragma once

#include <QDebug>
#include "launcherlog.h"
#include <QObject>

#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModDetails.h"

#include "tasks/Task.h"

class LocalModParseTask : public Task
{
    Q_OBJECT
public:
    struct Result {
        ModDetails details;
    };
    using ResultPtr = std::shared_ptr<Result>;
    ResultPtr result() const {
        return m_result;
    }

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override;

    LocalModParseTask(int token, ResourceType type, const QFileInfo & modFile);
    void executeTask() override;

    [[nodiscard]] int token() const { return m_token; }

private:
    void processAsZip();
    void processAsFolder();
    void processAsLitemod();

private:
    int m_token;
    ResourceType m_type;
    QFileInfo m_modFile;
    ResultPtr m_result;

    std::atomic<bool> m_aborted = false;
};
