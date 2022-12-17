// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

PostLaunchCommand::PostLaunchCommand(LaunchTask *parent) : LaunchStep(parent)
{
    auto instance = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->instance();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_command = instance->getPostExitCommand();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.setProcessEnvironment(instance->createEnvironment());
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process, &LoggedProcess::log, this, &PostLaunchCommand::logLines);
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process, &LoggedProcess::stateChanged, this, &PostLaunchCommand::on_state);
}

void PostLaunchCommand::executeTask()
{
    //FIXME: where to put this?
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    auto args = QProcess::splitCommand(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_command);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->substituteVariables(args);

    emit logLine(tr("Running Post-Launch command: %1").arg(args.join(' ')), MessageLevel::Launcher);
    const QString program = args.takeFirst();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.start(program, args);
#else
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->substituteVariables(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_command);

    emit logLine(tr("Running Post-Launch command: %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_command), MessageLevel::Launcher);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.start(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_command);
#endif
}

void PostLaunchCommand::on_state(LoggedProcess::State state)
{
    auto getError = [&]()
    {
        return tr("Post-Launch command failed with code %1.\n\n").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.exitCode());
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
            if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.exitCode() != 0)
            {
                auto error = getError();
                emit logLine(error, MessageLevel::Fatal);
                emitFailed(error);
            }
            else
            {
                emit logLine(tr("Post-Launch command ran successfully.\n\n"), MessageLevel::Launcher);
                emitSucceeded();
            }
        }
        default:
            break;
    }
}

void PostLaunchCommand::setWorkingDirectory(const QString &wd)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.setWorkingDirectory(wd);
}

bool PostLaunchCommand::abort()
{
    auto state = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.state();
    if (state == LoggedProcess::Running || state == LoggedProcess::Starting)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.kill();
    }
    return true;
}
