#include "ConcurrentTask.h"

#include <QDebug>
#include <QCoreApplication>
#include "tasks/Task.h"

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

auto ConcurrentTask::getStepProgress() const -> TaskStepProgressList
{
    return m_task_progress.values();
}

void ConcurrentTask::addTask(Task::Ptr task)
{
    m_queue.append(task);
}

void ConcurrentTask::executeTask()
{
    // Start the least amount of tasks needed, but at least one
    // int num_starts = qMax(1, qMin(m_total_max_size, m_queue.size()));
    // for (int i = 0; i < num_starts; i++) {
    //     QMetaObject::invokeMethod(this, &ConcurrentTask::startNext, Qt::QueuedConnection);
    // }
    // Start One task, startNext hadles starting the up to the m_total_max_size
    // while tracking the number currently being done
    QMetaObject::invokeMethod(this, &ConcurrentTask::startNext, Qt::QueuedConnection);
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

    connect(next.get(), &Task::succeeded, this, [this, next](){ subTaskSucceeded(next); });
    connect(next.get(), &Task::failed, this, [this, next](QString msg) { subTaskFailed(next, msg); });

    connect(next.get(), &Task::status, this, [this, next](QString msg){ subTaskStatus(next, msg); });
    connect(next.get(), &Task::stepProgress, this, [this, next](TaskStepProgressList tp){ subTaskStepProgress(next, tp); });

    connect(next.get(), &Task::progress, this, [this, next](qint64 current, qint64 total){ subTaskProgress(next, current, total); });

    m_doing.insert(next.get(), next);
    m_task_progress.insert(next->getUid(), std::make_shared<TaskStepProgress>(TaskStepProgress({next->getUid()}))); 


    updateState();
    updateStepProgress();

    QCoreApplication::processEvents();

    QMetaObject::invokeMethod(next.get(), &Task::start, Qt::QueuedConnection);

    // Allow going up the number of concurrent tasks in case of tasks being added in the middle of a running task.
    int num_starts = qMin(m_queue.size(), m_total_max_size - m_doing.size());
    for (int i = 0; i < num_starts; i++)
        QMetaObject::invokeMethod(this, &ConcurrentTask::startNext, Qt::QueuedConnection);
}

void ConcurrentTask::subTaskSucceeded(Task::Ptr task)
{
    m_done.insert(task.get(), task);
    m_succeeded.insert(task.get(), task);

    m_doing.remove(task.get());
    m_task_progress.value(task->getUid())->state = TaskStepState::Succeeded;

    disconnect(task.get(), 0, this, 0);

    updateState();
    updateStepProgress();
    startNext();
}

void ConcurrentTask::subTaskFailed(Task::Ptr task, const QString& msg)
{
    m_done.insert(task.get(), task);
    m_failed.insert(task.get(), task);

    m_doing.remove(task.get());
    m_task_progress.value(task->getUid())->state = TaskStepState::Failed;

    disconnect(task.get(), 0, this, 0);

    updateState();
    updateStepProgress();
    startNext();
}

void ConcurrentTask::subTaskStatus(Task::Ptr task, const QString& msg)
{
    auto taskProgress = m_task_progress.value(task->getUid());
    taskProgress->status = msg; 
    taskProgress->state = TaskStepState::Running;
    updateState();
    updateStepProgress();
}

void ConcurrentTask::subTaskProgress(Task::Ptr task, qint64 current, qint64 total)
{
    auto taskProgress = m_task_progress.value(task->getUid());
    
    taskProgress->current = current;
    taskProgress->total = total;
    taskProgress->state = TaskStepState::Running;
    taskProgress->details = task->getDetails(); 

    updateStepProgress();
    updateState();
}

void ConcurrentTask::subTaskStepProgress(Task::Ptr task, TaskStepProgressList task_step_progress)
{
    for (auto progress : task_step_progress) {
        if (!m_task_progress.contains(progress->uid)) {
            m_task_progress.insert(progress->uid, progress);
        } else {
            auto tp = m_task_progress.value(progress->uid);
            tp->current = progress->current;
            tp->total = progress->total;
            tp->status = progress->status;
            tp->details = progress->details;
        }           
    }

    updateStepProgress();
    
}

void ConcurrentTask::updateStepProgress()
{
   qint64 current = 0, total = 0;
   for ( auto taskProgress : m_task_progress ) {
       current += taskProgress->current;
       total += taskProgress->total;
   }

   m_stepProgress = current;
   m_stepTotalProgress = total;
   emit stepProgress(m_task_progress.values());
}

void ConcurrentTask::updateState()
{
    if (totalSize() > 1) {
        setProgress(m_done.count(), totalSize());
        setStatus(tr("Executing %1 task(s) (%2 out of %3 are done)").arg(QString::number(m_doing.count()), QString::number(m_done.count()), QString::number(totalSize())));
    } else {
        setProgress(m_stepProgress, m_stepTotalProgress);
        QString status = tr("Please wait ...");
        if (m_queue.size() > 0) {
            status = tr("Waiting for 1 task to start ...");
        } else if (m_doing.size() > 0) {
            status = tr("Executing 1 task:");
        } else if (m_done.size() > 0) {
            status = tr("Task finished.");
        }
        setStatus(status);
    }
}
