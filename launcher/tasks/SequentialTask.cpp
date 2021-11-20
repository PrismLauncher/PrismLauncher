#include "SequentialTask.h"

SequentialTask::SequentialTask(QObject *parent) : Task(parent), m_currentIndex(-1)
{
}

void SequentialTask::addTask(shared_qobject_ptr<Task> task)
{
    m_queue.append(task);
}

void SequentialTask::executeTask()
{
    m_currentIndex = -1;
    startNext();
}

void SequentialTask::startNext()
{
    if (m_currentIndex != -1)
    {
        shared_qobject_ptr<Task> previous = m_queue[m_currentIndex];
        disconnect(previous.get(), 0, this, 0);
    }
    m_currentIndex++;
    if (m_queue.isEmpty() || m_currentIndex >= m_queue.size())
    {
        emitSucceeded();
        return;
    }
    shared_qobject_ptr<Task> next = m_queue[m_currentIndex];
    connect(next.get(), SIGNAL(failed(QString)), this, SLOT(subTaskFailed(QString)));
    connect(next.get(), SIGNAL(status(QString)), this, SLOT(subTaskStatus(QString)));
    connect(next.get(), SIGNAL(progress(qint64, qint64)), this, SLOT(subTaskProgress(qint64, qint64)));
    connect(next.get(), SIGNAL(succeeded()), this, SLOT(startNext()));
    next->start();
}

void SequentialTask::subTaskFailed(const QString &msg)
{
    emitFailed(msg);
}
void SequentialTask::subTaskStatus(const QString &msg)
{
    setStatus(msg);
}
void SequentialTask::subTaskProgress(qint64 current, qint64 total)
{
    if(total == 0)
    {
        setProgress(0, 100);
        return;
    }
    setProgress(current, total);
}
