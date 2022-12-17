#pragma once
#include "tasks/Task.h"
#include "net/NetJob.h"
#include "minecraft/VersionFilterData.h"

class MinecraftInstance;

class FMLLibrariesTask : public Task
{
    Q_OBJECT
public:
    FMLLibrariesTask(MinecraftInstance * inst);
    virtual ~FMLLibrariesTask() {};

    void executeTask() override;

    bool canAbort() const override;

private slots:
    void fmllibsFinished();
    void fmllibsFailed(QString reason);

public slots:
    bool abort() override;

private:
    MinecraftInstance *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst;
    NetJob::Ptr downloadJob;
    QList<FMLlib> fmlLibsToProcess;
};

