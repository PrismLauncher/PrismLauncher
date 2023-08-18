// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "PostLaunchCommand.h"
#include <launch/LaunchTask.h>

PostLaunchCommand::PostLaunchCommand(LaunchTask* parent) : LaunchStep(parent)
{
    auto instance = m_parent->instance();
    m_command = instance->getPostExitCommand();
    m_process.setProcessEnvironment(instance->createEnvironment());
    connect(&m_process, &LoggedProcess::log, this, &PostLaunchCommand::logLines);
    connect(&m_process, &LoggedProcess::stateChanged, this, &PostLaunchCommand::on_state);
}

void PostLaunchCommand::executeTask()
{
    // FIXME: where to put this?
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    auto args = QProcess::splitCommand(m_command);
    m_parent->substituteVariables(args);

    emit logLine(tr("Running Post-Launch command: %1").arg(args.join(' ')), MessageLevel::Launcher);
    const QString program = args.takeFirst();
    m_process.start(program, args);
#else
    m_parent->substituteVariables(m_command);

    emit logLine(tr("Running Post-Launch command: %1").arg(m_command), MessageLevel::Launcher);
    m_process.start(m_command);
#endif
}

void PostLaunchCommand::on_state(LoggedProcess::State state)
{
    auto getError = [&]() { return tr("Post-Launch command failed with code %1.\n\n").arg(m_process.exitCode()); };
    switch (state) {
        case LoggedProcess::Aborted:
        case LoggedProcess::Crashed:
        case LoggedProcess::FailedToStart: {
            auto error = getError();
            emit logLine(error, MessageLevel::Fatal);
            emitFailed(error);
            return;
        }
        case LoggedProcess::Finished: {
            if (m_process.exitCode() != 0) {
                auto error = getError();
                emit logLine(error, MessageLevel::Fatal);
                emitFailed(error);
            } else {
                emit logLine(tr("Post-Launch command ran successfully.\n\n"), MessageLevel::Launcher);
                emitSucceeded();
            }
        }
        default:
            break;
    }
}

void PostLaunchCommand::setWorkingDirectory(const QString& wd)
{
    m_process.setWorkingDirectory(wd);
}

bool PostLaunchCommand::abort()
{
    auto state = m_process.state();
    if (state == LoggedProcess::Running || state == LoggedProcess::Starting) {
        m_process.kill();
    }
    return true;
}
