#include "InstanceCreationTask.h"

#include <QDebug>
#include <QFile>

InstanceCreationTask::InstanceCreationTask() = default;

void InstanceCreationTask::executeTask()
{
    setAbortable(true);

    if (updateInstance()) {
        emitSucceeded();
        return;
    }

    // When the user aborted in the update stage.
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abort) {
        emitAborted();
        return;
    }

    if (!createInstance()) {
        if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abort)
            return;

        qWarning() << "Instance creation failed!";
        if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_error_message.isEmpty()) {
            qWarning() << "Reason: " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_error_message;
            emitFailed(tr("Error while creating new instance:\n%1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_error_message));
        } else {
            emitFailed(tr("Error while creating new instance."));
        }

        return;
    }

    // If this is set, it means we're updating an instance. So, we now need to remove the
    // files scheduled to, and we'd better not let the user abort in the middle of it, since it'd
    // put the instance in an invalid state.
    if (shouldOverride()) {
        setAbortable(false);
        setStatus(tr("Removing old conflicting files..."));
        qDebug() << "Removing old files";

        for (auto path : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_files_to_remove) {
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
