#include "InstanceCreationTask.h"

#include <QDebug>

InstanceCreationTask::InstanceCreationTask() {}

void InstanceCreationTask::executeTask()
{
    if (updateInstance() || createInstance()) {
        emitSucceeded();
        return;
    }

    qWarning() << "Instance creation failed!";
    if (!m_error_message.isEmpty())
        qWarning() << "Reason: " << m_error_message;
    emitFailed(tr("Error while creating new instance."));
}
