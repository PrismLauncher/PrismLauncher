// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "Task.h"

#include <QDebug>

Q_LOGGING_CATEGORY(taskLogC, "launcher.task")

Task::Task(QObject *parent, bool show_debug) : QObject(parent), m_show_debug(show_debug)
{
    m_uid = QUuid::createUuid();
    setAutoDelete(false);
}

void Task::setStatus(const QString &new_status)
{
    if(m_status != new_status)
    {
        m_status = new_status;
        emit status(m_status);
    }
}

void Task::setDetails(const QString& new_details)
{
    if (m_details != new_details)
    {
        m_details = new_details;
        emit details(m_details);
    }
}

void Task::setProgress(qint64 current, qint64 total)
{
    if ((m_progress != current) || (m_progressTotal != total)) {
        m_progress = current;
        m_progressTotal = total;
        
        emit progress(m_progress, m_progressTotal);
    } 
}

void Task::start()
{
    switch(m_state)
    {
        case State::Inactive:
        {
            if (m_show_debug)
                qCDebug(taskLogC) << "Task" << describe() << "starting for the first time";
            break;
        }
        case State::AbortedByUser:
        {
            if (m_show_debug)
                qCDebug(taskLogC) << "Task" << describe() << "restarting for after being aborted by user";
            break;
        }
        case State::Failed:
        {
            if (m_show_debug)
                qCDebug(taskLogC) << "Task" << describe() << "restarting for after failing at first";
            break;
        }
        case State::Succeeded:
        {
            if (m_show_debug)
                qCDebug(taskLogC) << "Task" << describe() << "restarting for after succeeding at first";
            break;
        }
        case State::Running:
        {
            if (m_show_debug)
                qCWarning(taskLogC) << "The launcher tried to start task" << describe() << "while it was already running!";
            return;
        }
    }
    // NOTE: only fall thorugh to here in end states
    m_state = State::Running;
    emit started();
    executeTask();
}

void Task::emitFailed(QString reason)
{
    // Don't fail twice.
    if (!isRunning())
    {
        qCCritical(taskLogC) << "Task" << describe() << "failed while not running!!!!: " << reason;
        return;
    }
    m_state = State::Failed;
    m_failReason = reason;
    qCCritical(taskLogC) << "Task" << describe() << "failed: " << reason;
    emit failed(reason);
    emit finished();
}

void Task::emitAborted()
{
    // Don't abort twice.
    if (!isRunning())
    {
        qCCritical(taskLogC) << "Task" << describe() << "aborted while not running!!!!";
        return;
    }
    m_state = State::AbortedByUser;
    m_failReason = "Aborted.";
    if (m_show_debug)
        qCDebug(taskLogC) << "Task" << describe() << "aborted.";
    emit aborted();
    emit finished();
}

void Task::emitSucceeded()
{
    // Don't succeed twice.
    if (!isRunning())
    {
        qCCritical(taskLogC) << "Task" << describe() << "succeeded while not running!!!!";
        return;
    }
    m_state = State::Succeeded;
    if (m_show_debug)
        qCDebug(taskLogC) << "Task" << describe() << "succeeded";
    emit succeeded();
    emit finished();
}

void Task::propogateStepProgress(TaskStepProgress const& task_progress)
{
    emit stepProgress(task_progress);
}

QString Task::describe()
{
    QString outStr;
    QTextStream out(&outStr);
    out << metaObject()->className() << QChar('(');
    auto name = objectName();
    if(name.isEmpty())
    {
        out << QString("0x%1").arg(reinterpret_cast<quintptr>(this), 0, 16);
    }
    else
    {
        out << name;
    }
    out << " ID: " << m_uid.toString(QUuid::WithoutBraces);
    out << QChar(')');
    out.flush();
    return outStr;
}

bool Task::isRunning() const
{
    return m_state == State::Running;
}

bool Task::isFinished() const
{
    return m_state != State::Running && m_state != State::Inactive;
}

bool Task::wasSuccessful() const
{
    return m_state == State::Succeeded;
}

QString Task::failReason() const
{
    return m_failReason;
}

void Task::logWarning(const QString& line)
{
    qWarning() << line;
    m_Warnings.append(line);
}

QStringList Task::warnings() const
{
    return m_Warnings;
}
