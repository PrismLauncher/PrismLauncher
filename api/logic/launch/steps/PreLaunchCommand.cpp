/* Copyright 2013-2018 MultiMC Contributors
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

#include "PreLaunchCommand.h"
#include <launch/LaunchTask.h>

PreLaunchCommand::PreLaunchCommand(LaunchTask *parent) : LaunchStep(parent)
{
    auto instance = m_parent->instance();
    m_command = instance->getPreLaunchCommand();
    m_process.setProcessEnvironment(instance->createEnvironment());
    connect(&m_process, &LoggedProcess::log, this, &PreLaunchCommand::logLines);
    connect(&m_process, &LoggedProcess::stateChanged, this, &PreLaunchCommand::on_state);
}

void PreLaunchCommand::executeTask()
{
    //FIXME: where to put this?
    QString prelaunch_cmd = m_parent->substituteVariables(m_command);
    emit logLine(tr("Running Pre-Launch command: %1").arg(prelaunch_cmd), MessageLevel::MultiMC);
    m_process.start(prelaunch_cmd);
}

void PreLaunchCommand::on_state(LoggedProcess::State state)
{
    auto getError = [&]()
    {
        return tr("Pre-Launch command failed with code %1.\n\n").arg(m_process.exitCode());
    };
    switch(state)
    {
        case LoggedProcess::Aborted:
        case LoggedProcess::Crashed:
        case LoggedProcess::FailedToStart:
        {
            auto error = getError();
            emit logLine(error, MessageLevel::Fatal);
            emitFailed(error);
            return;
        }
        case LoggedProcess::Finished:
        {
            if(m_process.exitCode() != 0)
            {
                auto error = getError();
                emit logLine(error, MessageLevel::Fatal);
                emitFailed(error);
            }
            else
            {
                emit logLine(tr("Pre-Launch command ran successfully.\n\n"), MessageLevel::MultiMC);
                emitSucceeded();
            }
        }
        default:
            break;
    }
}

void PreLaunchCommand::setWorkingDirectory(const QString &wd)
{
    m_process.setWorkingDirectory(wd);
}

bool PreLaunchCommand::abort()
{
    auto state = m_process.state();
    if (state == LoggedProcess::Running || state == LoggedProcess::Starting)
    {
        m_process.kill();
    }
    return true;
}
