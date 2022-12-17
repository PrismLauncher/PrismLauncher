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

#include "MinecraftUpdate.h"
#include "MinecraftInstance.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>

#include "BaseInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/Library.h"
#include <FileSystem.h>

#include "update/FoldersTask.h"
#include "update/LibrariesTask.h"
#include "update/FMLLibrariesTask.h"
#include "update/AssetUpdateTask.h"

#include <meta/Index.h>
#include <meta/Version.h>

MinecraftUpdate::MinecraftUpdate(MinecraftInstance *inst, QObject *parent) : Task(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst(inst)
{
}

void MinecraftUpdate::executeTask()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.clear();
    // create folders
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.append(new FoldersTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst));
    }

    // add metadata update task if necessary
    {
        auto components = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst->getPackProfile();
        components->reload(Net::Mode::Online);
        auto task = components->getCurrentTask();
        if(task)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.append(task);
        }
    }

    // libraries download
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.append(new LibrariesTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst));
    }

    // FML libraries download and copy into the instance
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.append(new FMLLibrariesTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst));
    }

    // assets update
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.append(new AssetUpdateTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst));
    }

    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_preFailure.isEmpty())
    {
        emitFailed(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_preFailure);
        return;
    }
    next();
}

void MinecraftUpdate::next()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abort)
    {
        emitFailed(tr("Aborted by user."));
        return;
    }
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_out_of_order)
    {
        emitFailed(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fail_reason);
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentTask ++;
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentTask > 0)
    {
        auto task = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks[hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentTask - 1];
        disconnect(task.get(), &Task::succeeded, this, &MinecraftUpdate::subtaskSucceeded);
        disconnect(task.get(), &Task::failed, this, &MinecraftUpdate::subtaskFailed);
        disconnect(task.get(), &Task::aborted, this, &Task::abort);
        disconnect(task.get(), &Task::progress, this, &MinecraftUpdate::progress);
        disconnect(task.get(), &Task::status, this, &MinecraftUpdate::setStatus);
    }
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentTask == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks.size())
    {
        emitSucceeded();
        return;
    }
    auto task = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks[hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentTask];
    // if the task is already finished by the time we look at it, skip it
    if(task->isFinished())
    {
        qCritical() << "MinecraftUpdate: Skipping finished subtask" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentTask << ":" << task.get();
        next();
    }
    connect(task.get(), &Task::succeeded, this, &MinecraftUpdate::subtaskSucceeded);
    connect(task.get(), &Task::failed, this, &MinecraftUpdate::subtaskFailed);
    connect(task.get(), &Task::aborted, this, &Task::abort);
    connect(task.get(), &Task::progress, this, &MinecraftUpdate::progress);
    connect(task.get(), &Task::status, this, &MinecraftUpdate::setStatus);
    // if the task is already running, do not start it again
    if(!task->isRunning())
    {
        task->start();
    }
}

void MinecraftUpdate::subtaskSucceeded()
{
    if(isFinished())
    {
        qCritical() << "MinecraftUpdate: Subtask" << sender() << "succeeded, but work was already done!";
        return;
    }
    auto senderTask = QObject::sender();
    auto currentTask = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks[hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentTask].get();
    if(senderTask != currentTask)
    {
        qDebug() << "MinecraftUpdate: Subtask" << sender() << "succeeded out of order.";
        return;
    }
    next();
}

void MinecraftUpdate::subtaskFailed(QString error)
{
    if(isFinished())
    {
        qCritical() << "MinecraftUpdate: Subtask" << sender() << "failed, but work was already done!";
        return;
    }
    auto senderTask = QObject::sender();
    auto currentTask = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks[hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentTask].get();
    if(senderTask != currentTask)
    {
        qDebug() << "MinecraftUpdate: Subtask" << sender() << "failed out of order.";
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed_out_of_order = true;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fail_reason = error;
        return;
    }
    emitFailed(error);
}


bool MinecraftUpdate::abort()
{
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abort)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abort = true;
        auto task = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_tasks[hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentTask];
        if(task->canAbort())
        {
            return task->abort();
        }
    }
    return true;
}

bool MinecraftUpdate::canAbort() const
{
    return true;
}
