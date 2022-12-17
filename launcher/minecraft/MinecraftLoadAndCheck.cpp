#include "MinecraftLoadAndCheck.h"
#include "MinecraftInstance.h"
#include "PackProfile.h"

MinecraftLoadAndCheck::MinecraftLoadAndCheck(MinecraftInstance *inst, QObject *parent) : Task(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst(inst)
{
}

void MinecraftLoadAndCheck::executeTask()
{
    // add offline metadata load task
    auto components = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst->getPackProfile();
    components->reload(Net::Mode::Offline);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task = components->getCurrentTask();

    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task)
    {
        emitSucceeded();
        return;
    }
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task.get(), &Task::succeeded, this, &MinecraftLoadAndCheck::subtaskSucceeded);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task.get(), &Task::failed, this, &MinecraftLoadAndCheck::subtaskFailed);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task.get(), &Task::aborted, this, [this]{ subtaskFailed(tr("Aborted")); });
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task.get(), &Task::progress, this, &MinecraftLoadAndCheck::progress);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task.get(), &Task::status, this, &MinecraftLoadAndCheck::setStatus);
}

void MinecraftLoadAndCheck::subtaskSucceeded()
{
    if(isFinished())
    {
        qCritical() << "MinecraftUpdate: Subtask" << sender() << "succeeded, but work was already done!";
        return;
    }
    emitSucceeded();
}

void MinecraftLoadAndCheck::subtaskFailed(QString error)
{
    if(isFinished())
    {
        qCritical() << "MinecraftUpdate: Subtask" << sender() << "failed, but work was already done!";
        return;
    }
    emitFailed(error);
}
