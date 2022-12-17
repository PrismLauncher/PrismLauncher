#include "MultipleOptionsTask.h"

#include <QDebug>

MultipleOptionsTask::MultipleOptionsTask(QObject* parent, const QString& task_name) : SequentialTask(parent, task_name) {}

void MultipleOptionsTask::startNext()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.size() != hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed.size()) {
        emitSucceeded();
        return;
    }

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.isEmpty()) {
        emitFailed(tr("All attempts have failed!"));
        qWarning() << "All attempts have failed!";
        return;
    }

    ConcurrentTask::startNext();
}

void MultipleOptionsTask::updateState()
{
    setProgress(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.count(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size);
    setStatus(tr("Attempting task %1 out of %2").arg(QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.count() + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.count()), QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size)));
}
