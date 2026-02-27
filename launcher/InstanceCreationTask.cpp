#include "InstanceCreationTask.h"

#include <QDebug>
#include <QFile>

#include "minecraft/MinecraftLoadAndCheck.h"
#include "tasks/SequentialTask.h"

bool InstanceCreationTask::abort()
{
    if (!canAbort()) {
        return false;
    }

    m_abort = true;
    if (m_gameFilesTask) {
        return m_gameFilesTask->abort();
    }

    return true;
}

void InstanceCreationTask::executeTask()
{
    setAbortable(true);

    if (updateInstance()) {
        emitSucceeded();
        return;
    }

    // When the user aborted in the update stage.
    if (m_abort) {
        emitAborted();
        return;
    }

    m_instance = createInstance();
    if (!m_instance) {
        if (m_abort)
            return;

        qWarning() << "Instance creation failed!";
        if (!m_error_message.isEmpty()) {
            qWarning() << "Reason:" << m_error_message;
            emitFailed(tr("Error while creating new instance:\n%1").arg(m_error_message));
        } else {
            emitFailed(tr("Error while creating new instance."));
        }

        return;
    }

    // If this is set, it means we're updating an instance. So, we now need to remove the
    // files scheduled to, and we'd better not let the user abort in the middle of it, since it'd
    // put the instance in an invalid state.
    if (shouldOverride()) {
        bool deleteFailed = false;

        setAbortable(false);
        setStatus(tr("Removing old conflicting files..."));
        qDebug() << "Removing old files";

        for (const QString& path : m_files_to_remove) {
            if (!QFile::exists(path))
                continue;

            qDebug() << "Removing" << path;

            if (!QFile::remove(path)) {
                qCritical() << "Could not remove" << path;
                deleteFailed = true;
            }
        }

        if (deleteFailed) {
            emitFailed(tr("Failed to remove old conflicting files."));
            return;
        }
    }

    if (!m_abort) {
        setAbortable(true);
        setAbortButtonText(tr("Skip"));
        qDebug() << "Downloading game files";

        auto updateTasks = m_instance->createUpdateTask();
        if (updateTasks.isEmpty()) {
            emitSucceeded();
            return;
        }
        auto task = makeShared<SequentialTask>();
        task->addTask(makeShared<MinecraftLoadAndCheck>(m_instance.get(), Net::Mode::Online));
        for (const auto& t : updateTasks) {
            task->addTask(t);
        }
        connect(task.get(), &Task::finished, this, [this, task] {
            if (task->wasSuccessful() || m_abort) {
                emitSucceeded();
            } else {
                emitFailed(tr("Could not download game files: %1").arg(task->failReason()));
            }
        });
        propagateFromOther(task.get());
        setDetails(tr("Downloading game files"));

        m_gameFilesTask = task;
        m_gameFilesTask->start();
    }
}
