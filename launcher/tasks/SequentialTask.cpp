#include "SequentialTask.h"

#include <QDebug>

SequentialTask::SequentialTask(QObject* parent, QString task_name) : ConcurrentTask(parent, task_name, 1) {}

void SequentialTask::startNext()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed.size() > 0) {
        emitFailed(tr("One of the tasks failed!"));
        qWarning() << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed.constBegin()->get()->failReason();
        return;
    }

    ConcurrentTask::startNext();
}

void SequentialTask::updateState()
{
    setProgress(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.count(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size);
    setStatus(tr("Executing task %1 out of %2").arg(QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.count() + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.count()), QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size)));
}
