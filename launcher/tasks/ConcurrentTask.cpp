#include "ConcurrentTask.h"

#include <QDebug>
#include <QCoreApplication>

ConcurrentTask::ConcurrentTask(QObject* parent, QString task_name, int max_concurrent)
    : Task(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name(task_name), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_max_size(max_concurrent)
{ setObjectName(task_name); }

ConcurrentTask::~ConcurrentTask()
{
    for (auto task : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue) {
        if (task)
            task->deleteLater();
    }
}

auto ConcurrentTask::getStepProgress() const -> qint64
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stepProgress;
}

auto ConcurrentTask::getStepTotalProgress() const -> qint64
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stepTotalProgress;
}

void ConcurrentTask::addTask(Task::Ptr task)
{
    if (!isRunning())
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.append(task);
    else
        qWarning() << "Tried to add a task to a running concurrent task!";
}

void ConcurrentTask::executeTask()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.size();

    // Start the least amount of tasks needed, but at least one
    int nuhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_starts = std::max(1, std::min(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_max_size, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size));
    for (int i = 0; i < nuhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_starts; i++) {
        QMetaObject::invokeMethod(this, &ConcurrentTask::startNext, Qt::QueuedConnection);
    }
}

bool ConcurrentTask::abort()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.clear();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = true;

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.isEmpty()) {
        // Don't call emitAborted() here, we want to bypass the 'is the task running' check
        emit aborted();
        emit finished();

        return true;
    }

    bool suceedeed = true;

    QMutableHashIterator<Task*, Task::Ptr> doing_iter(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing);
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

void ConcurrentTask::startNext()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.count() > hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_max_size)
        return;

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.isEmpty() && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.isEmpty() && !wasSuccessful()) {
        emitSucceeded();
        return;
    }

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.isEmpty())
        return;

    Task::Ptr next = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.dequeue();

    connect(next.get(), &Task::succeeded, this, [this, next] { subTaskSucceeded(next); });
    connect(next.get(), &Task::failed, this, [this, next](QString msg) { subTaskFailed(next, msg); });

    connect(next.get(), &Task::status, this, &ConcurrentTask::subTaskStatus);
    connect(next.get(), &Task::stepStatus, this, &ConcurrentTask::subTaskStatus);

    connect(next.get(), &Task::progress, this, &ConcurrentTask::subTaskProgress);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.insert(next.get(), next);

    setStepStatus(next->isMultiStep() ? next->getStepStatus() : next->getStatus());
    updateState();

    QCoreApplication::processEvents();

    next->start();
}

void ConcurrentTask::subTaskSucceeded(Task::Ptr task)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.insert(task.get(), task);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.remove(task.get());

    disconnect(task.get(), 0, this, 0);

    updateState();

    startNext();
}

void ConcurrentTask::subTaskFailed(Task::Ptr task, const QString& msg)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.insert(task.get(), task);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed.insert(task.get(), task);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.remove(task.get());

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
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stepProgress = current;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stepTotalProgress = total;
}

void ConcurrentTask::updateState()
{
    setProgress(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.count(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size);
    setStatus(tr("Executing %1 task(s) (%2 out of %3 are done)")
                  .arg(QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.count()), QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.count()), QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size)));
}
