#include "MinecraftLoadAndCheck.h"
#include "MinecraftInstance.h"
#include "PackProfile.h"

MinecraftLoadAndCheck::MinecraftLoadAndCheck(MinecraftInstance *inst, QObject *parent) : Task(parent), m_inst(inst)
{
}

void MinecraftLoadAndCheck::executeTask()
{
    // add offline metadata load task
    auto components = m_inst->getPackProfile();
    components->reload(Net::Mode::Offline);
    m_task = components->getCurrentTask();

    if(!m_task)
    {
        emitSucceeded();
        return;
    }
    connect(m_task.get(), &Task::succeeded, this, &MinecraftLoadAndCheck::subtaskSucceeded);
    connect(m_task.get(), &Task::failed, this, &MinecraftLoadAndCheck::subtaskFailed);
    connect(m_task.get(), &Task::aborted, this, [this]{ subtaskFailed(tr("Aborted")); });
    connect(m_task.get(), &Task::progress, this, &MinecraftLoadAndCheck::progress);
    connect(m_task.get(), &Task::stepProgress, this, &MinecraftLoadAndCheck::propogateStepProgress);
    connect(m_task.get(), &Task::status, this, &MinecraftLoadAndCheck::setStatus);
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
