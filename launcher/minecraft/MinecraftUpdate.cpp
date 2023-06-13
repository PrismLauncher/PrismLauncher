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

MinecraftUpdate::MinecraftUpdate(MinecraftInstance *inst, QObject *parent) : Task(parent), m_inst(inst)
{
}

void MinecraftUpdate::executeTask()
{
    m_tasks.clear();
    // create folders
    {
        m_tasks.append(makeShared<FoldersTask>(m_inst));
    }

    // add metadata update task if necessary
    {
        auto components = m_inst->getPackProfile();
        components->reload(Net::Mode::Online);
        auto task = components->getCurrentTask();
        if(task)
        {
            m_tasks.append(task);
        }
    }

    // libraries download
    {
        m_tasks.append(makeShared<LibrariesTask>(m_inst));
    }

    // FML libraries download and copy into the instance
    {
        m_tasks.append(makeShared<FMLLibrariesTask>(m_inst));
    }

    // assets update
    {
        m_tasks.append(makeShared<AssetUpdateTask>(m_inst));
    }

    if(!m_preFailure.isEmpty())
    {
        emitFailed(m_preFailure);
        return;
    }
    next();
}

void MinecraftUpdate::next()
{
    if(m_abort)
    {
        emitFailed(tr("Aborted by user."));
        return;
    }
    if(m_failed_out_of_order)
    {
        emitFailed(m_fail_reason);
        return;
    }
    m_currentTask ++;
    if(m_currentTask > 0)
    {
        auto task = m_tasks[m_currentTask - 1];
        disconnect(task.get(), &Task::succeeded, this, &MinecraftUpdate::subtaskSucceeded);
        disconnect(task.get(), &Task::failed, this, &MinecraftUpdate::subtaskFailed);
        disconnect(task.get(), &Task::aborted, this, &Task::abort);
        disconnect(task.get(), &Task::progress, this, &MinecraftUpdate::progress);
        disconnect(task.get(), &Task::stepProgress, this, &MinecraftUpdate::propogateStepProgress);
        disconnect(task.get(), &Task::status, this, &MinecraftUpdate::setStatus);
        disconnect(task.get(), &Task::details, this, &MinecraftUpdate::setDetails);
    }
    if(m_currentTask == m_tasks.size())
    {
        emitSucceeded();
        return;
    }
    auto task = m_tasks[m_currentTask];
    // if the task is already finished by the time we look at it, skip it
    if(task->isFinished())
    {
        qCritical() << "MinecraftUpdate: Skipping finished subtask" << m_currentTask << ":" << task.get();
        next();
    }
    connect(task.get(), &Task::succeeded, this, &MinecraftUpdate::subtaskSucceeded);
    connect(task.get(), &Task::failed, this, &MinecraftUpdate::subtaskFailed);
    connect(task.get(), &Task::aborted, this, &Task::abort);
    connect(task.get(), &Task::progress, this, &MinecraftUpdate::progress);
    connect(task.get(), &Task::stepProgress, this, &MinecraftUpdate::propogateStepProgress);
    connect(task.get(), &Task::status, this, &MinecraftUpdate::setStatus);
    connect(task.get(), &Task::details, this, &MinecraftUpdate::setDetails);
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
    auto currentTask = m_tasks[m_currentTask].get();
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
    auto currentTask = m_tasks[m_currentTask].get();
    if(senderTask != currentTask)
    {
        qDebug() << "MinecraftUpdate: Subtask" << sender() << "failed out of order.";
        m_failed_out_of_order = true;
        m_fail_reason = error;
        return;
    }
    emitFailed(error);
}


bool MinecraftUpdate::abort()
{
    if(!m_abort)
    {
        m_abort = true;
        auto task = m_tasks[m_currentTask];
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
