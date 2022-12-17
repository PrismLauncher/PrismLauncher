#pragma once
#include "tasks/Task.h"
#include "net/NetJob.h"
class MinecraftInstance;

class LibrariesTask : public Task
{
    Q_OBJECT
public:
    LibrariesTask(MinecraftInstance * inst);
    virtual ~LibrariesTask() {};

    void executeTask() override;

    bool canAbort() const override;

private slots:
    void jarlibFailed(QString reason);

public slots:
    bool abort() override;

private:
    MinecraftInstance *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst;
    NetJob::Ptr downloadJob;
};
