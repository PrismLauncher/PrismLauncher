#include "InstanceCreationTask.h"

#include <QDebug>

InstanceCreationTask::InstanceCreationTask() = default;

void InstanceCreationTask::executeTask()
{
    if (updateInstance()) {
        emitSucceeded();
        return;
    }

    // When the user aborted in the update stage.
    if (m_abort) {
        emitAborted();
        return;
    }

    // If this is set, it means we're updating an instance. Since the previous step likely
    // removed some old files, we'd better not let the user abort the next task, since it'd
    // put the instance in an invalid state.
    // TODO: Figure out an unexpensive way of making such file removal a recoverable transaction.
    setAbortStatus(!shouldOverride());

    if (createInstance()) {
        emitSucceeded();
        return;
    }

    qWarning() << "Instance creation failed!";
    if (!m_error_message.isEmpty())
        qWarning() << "Reason: " << m_error_message;
    emitFailed(tr("Error while creating new instance."));
}
