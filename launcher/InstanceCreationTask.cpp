#include "InstanceCreationTask.h"

#include <QFile>

void InstanceCreationTask::finishTask()
{
    // If this is set, it means we're updating an instance. So, we now need to remove the
    // files scheduled to, and we'd better not let the user abort in the middle of it, since it'd
    // put the instance in an invalid state.
    if (shouldOverride()) {
        setAbortable(false);
        setStatus(tr("Removing old conflicting files..."));
        qDebug() << "Removing old files";

        for (auto path : m_files_to_remove) {
            if (!QFile::exists(path))
                continue;
            qDebug() << "Removing" << path;
            if (!QFile::remove(path)) {
                qCritical() << "Couldn't remove the old conflicting files.";
                emitFailed(tr("Failed to remove old conflicting files."));
                return;
            }
        }
    }

    emitSucceeded();
    return;
}

void InstanceCreationTask::emitFailed(QString reason)
{
    qWarning() << "Instance creation failed!";
    if (!reason.isEmpty()) {
        qWarning() << "Reason: " << reason;
        reason = tr("Error while creating new instance:\n%1").arg(reason);
    } else {
        reason = tr("Error while creating new instance.");
    }
    Task::emitFailed(reason);
}
