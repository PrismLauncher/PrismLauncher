#include "MultipleOptionsTask.h"

#include <QDebug>

MultipleOptionsTask::MultipleOptionsTask(QObject* parent, const QString& task_name) : SequentialTask(parent, task_name) {}

void MultipleOptionsTask::startNext()
{
    Task* previous = nullptr;
    if (m_currentIndex != -1) {
        previous = m_queue[m_currentIndex].get();
        disconnect(previous, 0, this, 0);
    }

    m_currentIndex++;
    if ((previous && previous->wasSuccessful())) {
        emitSucceeded();
        return;
    }

    Task::Ptr next = m_queue[m_currentIndex];

    connect(next.get(), &Task::failed, this, &MultipleOptionsTask::subTaskFailed);
    connect(next.get(), &Task::succeeded, this, &MultipleOptionsTask::startNext);

    connect(next.get(), &Task::status, this, &MultipleOptionsTask::subTaskStatus);
    connect(next.get(), &Task::stepStatus, this, &MultipleOptionsTask::subTaskStatus);
    
    connect(next.get(), &Task::progress, this, &MultipleOptionsTask::subTaskProgress);

    qDebug() << QString("Making attemp %1 out of %2").arg(m_currentIndex + 1).arg(m_queue.size());
    setStatus(tr("Making attempt #%1 out of %2").arg(m_currentIndex + 1).arg(m_queue.size()));
    setStepStatus(next->isMultiStep() ? next->getStepStatus() : next->getStatus());

    next->start();
}

void MultipleOptionsTask::subTaskFailed(QString const& reason)
{
    qDebug() << QString("Failed attempt #%1 of %2. Reason: %3").arg(m_currentIndex + 1).arg(m_queue.size()).arg(reason);
    if(m_currentIndex < m_queue.size() - 1) {
        startNext();
        return;
    }

    qWarning() << QString("All attempts have failed!");
    emitFailed();
}
