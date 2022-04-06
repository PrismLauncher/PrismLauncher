#include "SequentialTask.h"

SequentialTask::SequentialTask(QObject* parent, const QString& task_name) : Task(parent), m_name(task_name), m_currentIndex(-1) {}

SequentialTask::~SequentialTask()
{
    for(auto task : m_queue){
        if(task)
            task->deleteLater();
    }
}

auto SequentialTask::getStepProgress() const -> qint64
{
    return m_stepProgress;
}

auto SequentialTask::getStepTotalProgress() const -> qint64
{
    return m_stepTotalProgress;
}

void SequentialTask::addTask(Task::Ptr task)
{
    m_queue.append(task);
}

void SequentialTask::executeTask()
{
    m_currentIndex = -1;
    startNext();
}

bool SequentialTask::abort()
{
    bool succeeded = true;
    for (auto& task : m_queue) {
        if (!task->abort()) succeeded = false;
    }

    return succeeded;
}

void SequentialTask::startNext()
{
    if (m_currentIndex != -1) {
        Task::Ptr previous = m_queue[m_currentIndex];
        disconnect(previous.get(), 0, this, 0);
    }
    m_currentIndex++;
    if (m_queue.isEmpty() || m_currentIndex >= m_queue.size()) {
        emitSucceeded();
        return;
    }
    Task::Ptr next = m_queue[m_currentIndex];
    connect(next.get(), SIGNAL(failed(QString)), this, SLOT(subTaskFailed(QString)));
    connect(next.get(), SIGNAL(status(QString)), this, SLOT(subTaskStatus(QString)));
    connect(next.get(), SIGNAL(progress(qint64, qint64)), this, SLOT(subTaskProgress(qint64, qint64)));
    connect(next.get(), SIGNAL(succeeded()), this, SLOT(startNext()));

    setStatus(tr("Executing task %1 out of %2").arg(m_currentIndex + 1).arg(m_queue.size()));
    next->start();
}

void SequentialTask::subTaskFailed(const QString& msg)
{
    emitFailed(msg);
}
void SequentialTask::subTaskStatus(const QString& msg)
{
    setStepStatus(m_queue[m_currentIndex]->getStatus());
}
void SequentialTask::subTaskProgress(qint64 current, qint64 total)
{
    if (total == 0) {
        setProgress(0, 100);
        return;
    }
    setProgress(m_currentIndex, m_queue.count());

    m_stepProgress = current;
    m_stepTotalProgress = total;
}
