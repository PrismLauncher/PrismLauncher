#pragma once
#include "tasks/Task.h"
#include "net/NetJob.h"
class MinecraftInstance;

class AssetUpdateTask : public Task
{
    Q_OBJECT
public:
    AssetUpdateTask(MinecraftInstance * inst);
    virtual ~AssetUpdateTask();

    void executeTask() override;

    bool canAbort() const override;

private slots:
    void assetIndexFinished();
    void assetIndexFailed(QString reason);
    void assetsFailed(QString reason);

public slots:
    bool abort() override;

private:
    MinecraftInstance *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst;
    NetJob::Ptr downloadJob;
};
