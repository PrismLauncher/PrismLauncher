#include "ConcurrentTask.h"

#include <QDebug>
#include <QCoreApplication>

ConcurrentTask::ConcurrentTask(QObject* parent, QString task_name, int max_concurrent)
    : Task(parent), m_name(task_name), m_total_max_size(max_concurrent)
{ setObjectName(task_name); }

ConcurrentTask::~ConcurrentTask()
{
    for (auto task : m_queue) {
        if (task)
            task->deleteLater();
    }
}

auto ConcurrentTask::getStepProgress() const -> qint64
{
    return m_stepProgress;
}

auto ConcurrentTask::getStepTotalProgress() const -> qint64
{
    return m_stepTotalProgress;
}

void ConcurrentTask::addTask(Task::Ptr task)
{
    m_queue.append(task);
}

void ConcurrentTask::executeTask()
{
    // Start the least amount of tasks needed, but at least one
    int num_starts = qMax(1, qMin(m_total_max_size, m_queue.size()));
    for (int i = 0; i < num_starts; i++) {
        QMetaObject::invokeMethod(this, &ConcurrentTask::startNext, Qt::QueuedConnection);
    }
}

bool ConcurrentTask::abort()
{
    m_queue.clear();
    m_aborted = true;

    if (m_doing.isEmpty()) {
        // Don't call emitAborted() here, we want to bypass the 'is the task running' check
        emit aborted();
        emit finished();

        return true;
    }

    bool suceedeed = true;

    QMutableHashIterator<Task*, Task::Ptr> doing_iter(m_doing);
    while (doing_iter.hasNext()) {
        auto task = doing_iter.next();
        suceedeed &= (task.value())->abort();
    }

    if (suceedeed)
        emitAborted();
    else
        emitFailed(tr("Failed to abort all running tasks."));

    return suceedeed;
}

void ConcurrentTask::clear()
{
    Q_ASSERT(!isRunning());

    m_done.clear();
    m_failed.clear();
    m_queue.clear();

    m_aborted = false;

    m_progress = 0;
    m_stepProgress = 0;
}

void ConcurrentTask::startNext()
{
    if (m_aborted || m_doing.count() > m_total_max_size)
        return;

    if (m_queue.isEmpty() && m_doing.isEmpty() && !wasSuccessful()) {
        emitSucceeded();
        return;
    }

    if (m_queue.isEmpty())
        return;

    Task::Ptr next = m_queue.dequeue();

    connect(next.get(), &Task::succeeded, this, [this, next] { subTaskSucceeded(next); });
    connect(next.get(), &Task::failed, this, [this, next](QString msg) { subTaskFailed(next, msg); });

    connect(next.get(), &Task::status, this, &ConcurrentTask::subTaskStatus);
    connect(next.get(), &Task::stepStatus, this, &ConcurrentTask::subTaskStatus);

    connect(next.get(), &Task::progress, this, &ConcurrentTask::subTaskProgress);

    m_doing.insert(next.get(), next);

    setStepStatus(next->isMultiStep() ? next->getStepStatus() : next->getStatus());
    updateState();

    QMetaObject::invokeMethod(next.get(), &Task::start, Qt::QueuedConnection);

    // Allow going up the number of concurrent tasks in case of tasks being added in the middle of a running task.
    int num_starts = m_total_max_size - m_doing.size();
    for (int i = 0; i < num_starts; i++)
        QMetaObject::invokeMethod(this, &ConcurrentTask::startNext, Qt::QueuedConnection);

    QCoreApplication::processEvents();
}

void ConcurrentTask::subTaskSucceeded(Task::Ptr task)
{
    m_done.insert(task.get(), task);
    m_doing.remove(task.get());

    disconnect(task.get(), 0, this, 0);

    updateState();

    startNext();
}

void ConcurrentTask::subTaskFailed(Task::Ptr task, const QString& msg)
{
    m_done.insert(task.get(), task);
    m_failed.insert(task.get(), task);

    m_doing.remove(task.get());

    disconnect(task.get(), 0, this, 0);

    updateState();

    startNext();
}

void ConcurrentTask::subTaskStatus(const QString& msg)
{
    setStepStatus(msg);
}

void ConcurrentTask::subTaskProgress(qint64 current, qint64 total)
{
    m_stepProgress = current;
    m_stepTotalProgress = total;
}

void ConcurrentTask::updateState()
{
    setProgress(m_done.count(), totalSize());
    setStatus(tr("Executing %1 task(s) (%2 out of %3 are done)")
                  .arg(QString::number(m_doing.count()), QString::number(m_done.count()), QString::number(totalSize())));
}
