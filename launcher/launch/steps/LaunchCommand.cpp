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

#include "LaunchCommand.h"
#include <launch/LaunchTask.h>

#include <utility>

LaunchCommand::LaunchCommand(LaunchTask* parent, QString command, QString phaseName)
    : LaunchStep(parent), m_command(std::move(command)), m_phaseName(std::move(phaseName))
{
    auto* instance = m_parent->instance();
    m_process.setProcessEnvironment(instance->createEnvironment());
    connect(&m_process, &LoggedProcess::log, this, &LaunchCommand::logLines);
    connect(&m_process, &LoggedProcess::stateChanged, this, &LaunchCommand::onState);
}

void LaunchCommand::executeTask()
{
    auto cmd = m_parent->substituteVariables(m_command);
    emit logLine(tr("Running %1 command: %2").arg(m_phaseName, cmd), MessageLevel::Launcher);
    auto args = QProcess::splitCommand(cmd);

    const QString program = args.takeFirst();
    m_process.start(program, args);
}

void LaunchCommand::onState(LoggedProcess::State state)
{
    auto getError = [this]() {
        auto error = tr("%1 command failed with code %2.\n\n").arg(m_phaseName).arg(m_process.exitCode());
        return error;
    };
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
                emit logLine(tr("%1 command ran successfully.\n\n").arg(m_phaseName), MessageLevel::Launcher);
                emitSucceeded();
            }
        }
        default:
            break;
    }
}

void LaunchCommand::setWorkingDirectory(const QString& wd)
{
    m_process.setWorkingDirectory(wd);
}

bool LaunchCommand::abort()
{
    auto state = m_process.state();
    if (state == LoggedProcess::Running || state == LoggedProcess::Starting) {
        m_process.kill();
    }
    return true;
}
