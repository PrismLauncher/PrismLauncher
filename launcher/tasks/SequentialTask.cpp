#include "SequentialTask.h"

#include <QDebug>

SequentialTask::SequentialTask(QObject* parent, QString task_name) : ConcurrentTask(parent, task_name, 1) {}

void SequentialTask::startNext()
{
    if (m_failed.size() > 0) {
        emitFailed(tr("One of the tasks failed!"));
        qWarning() << m_failed.constBegin()->get()->failReason();
        return;
    }

    ConcurrentTask::startNext();
}

void SequentialTask::updateState()
{
    setProgress(m_done.count(), totalSize());
    setStatus(tr("Executing task %1 out of %2").arg(QString::number(m_doing.count() + m_done.count()), QString::number(totalSize())));
}
