// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

Task::Task(QObject *parent, bool show_debug) : QObject(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_show_debug(show_debug)
{
    setAutoDelete(false);
}

void Task::setStatus(const QString &new_status)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status != new_status)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status = new_status;
        emit status(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status);
    }
}

void Task::setProgress(qint64 current, qint64 total)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progress = current;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progressTotal = total;
    emit progress(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progress, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progressTotal);
}

void Task::start()
{
    switch(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state)
    {
        case State::Inactive:
        {
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_show_debug)
                qDebug() << "Task" << describe() << "starting for the first time";
            break;
        }
        case State::AbortedByUser:
        {
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_show_debug)
                qDebug() << "Task" << describe() << "restarting for after being aborted by user";
            break;
        }
        case State::Failed:
        {
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_show_debug)
                qDebug() << "Task" << describe() << "restarting for after failing at first";
            break;
        }
        case State::Succeeded:
        {
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_show_debug)
                qDebug() << "Task" << describe() << "restarting for after succeeding at first";
            break;
        }
        case State::Running:
        {
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_show_debug)
                qWarning() << "The launcher tried to start task" << describe() << "while it was already running!";
            return;
        }
    }
    // NOTE: only fall thorugh to here in end states
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Running;
    emit started();
    executeTask();
}

void Task::emitFailed(QString reason)
{
    // Don't fail twice.
    if (!isRunning())
    {
        qCritical() << "Task" << describe() << "failed while not running!!!!: " << reason;
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Failed;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failReason = reason;
    qCritical() << "Task" << describe() << "failed: " << reason;
    emit failed(reason);
    emit finished();
}

void Task::emitAborted()
{
    // Don't abort twice.
    if (!isRunning())
    {
        qCritical() << "Task" << describe() << "aborted while not running!!!!";
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::AbortedByUser;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failReason = "Aborted.";
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_show_debug)
        qDebug() << "Task" << describe() << "aborted.";
    emit aborted();
    emit finished();
}

void Task::emitSucceeded()
{
    // Don't succeed twice.
    if (!isRunning())
    {
        qCritical() << "Task" << describe() << "succeeded while not running!!!!";
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Succeeded;
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_show_debug)
        qDebug() << "Task" << describe() << "succeeded";
    emit succeeded();
    emit finished();
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
    out << QChar(')');
    out.flush();
    return outStr;
}

bool Task::isRunning() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state == State::Running;
}

bool Task::isFinished() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state != State::Running && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state != State::Inactive;
}

bool Task::wasSuccessful() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state == State::Succeeded;
}

QString Task::failReason() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failReason;
}

void Task::logWarning(const QString& line)
{
    qWarning() << line;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_Warnings.append(line);
}

QStringList Task::warnings() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_Warnings;
}
