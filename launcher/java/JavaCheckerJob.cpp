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
    num_finished++;
    qDebug() << m_job_name.toLocal8Bit() << "progress:" << num_finished << "/"
                << javacheckers.size();
    setProgress(num_finished, javacheckers.size());

    javaresults.replace(result.id, result);

    if (num_finished == javacheckers.size())
    {
        emitSucceeded();
    }
}

void JavaCheckerJob::executeTask()
{
    qDebug() << m_job_name.toLocal8Bit() << " started.";
    for (auto iter : javacheckers)
    {
        javaresults.append(JavaCheckResult());
        connect(iter.get(), &JavaChecker::checkFinished, this, &JavaCheckerJob::partFinished);
        iter->performCheck();
    }
}
