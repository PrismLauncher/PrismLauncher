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

#include "JavaCheckerJob.h"

#include <QDebug>

void JavaCheckerJob::partFinished(JavaCheckResult result)
{
    nuhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_finished++;
    qDebug() << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_job_name.toLocal8Bit() << "progress:" << nuhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_finished << "/"
                << javacheckers.size();
    setProgress(nuhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_finished, javacheckers.size());

    javaresults.replace(result.id, result);

    if (nuhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_finished == javacheckers.size())
    {
        emitSucceeded();
    }
}

void JavaCheckerJob::executeTask()
{
    qDebug() << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_job_name.toLocal8Bit() << " started.";
    for (auto iter : javacheckers)
    {
        javaresults.append(JavaCheckResult());
        connect(iter.get(), SIGNAL(checkFinished(JavaCheckResult)), SLOT(partFinished(JavaCheckResult)));
        iter->performCheck();
    }
}
