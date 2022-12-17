/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Update.h"
#include <launch/LaunchTask.h>

void Update::executeTask()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted)
    {
        emitFailed(tr("Task aborted."));
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.reset(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->instance()->createUpdateTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mode));
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask)
    {
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.get(), SIGNAL(finished()), this, SLOT(updateFinished()));
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.get(), &Task::progress, this, &Task::setProgress);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.get(), &Task::status, this, &Task::setStatus);
        emit progressReportingRequest();
        return;
    }
    emitSucceeded();
}

void Update::proceed()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask->start();
}

void Update::updateFinished()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask->wasSuccessful())
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.reset();
        emitSucceeded();
    }
    else
    {
        QString reason = tr("Instance update failed because: %1\n\n").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask->failReason());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.reset();
        emit logLine(reason, MessageLevel::Fatal);
        emitFailed(reason);
    }
}

bool Update::canAbort() const
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask)
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask->canAbort();
    }
    return true;
}


bool Update::abort()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = true;
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask)
    {
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask->canAbort())
        {
            return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask->abort();
        }
    }
    return true;
}
