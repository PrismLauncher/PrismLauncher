// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
#include "ConcurrentTask.h"

#include <QDebug>
#include <QCoreApplication>
#include "tasks/Task.h"

ConcurrentTask::ConcurrentTask(QObject* parent, QString task_name, int max_concurrent)
    : Task(parent), m_name(task_name), m_total_max_size(max_concurrent)
{ setObjectName(task_name); }

ConcurrentTask::~ConcurrentTask()
{
    for (auto task : m_queue) {
        if (task)
            task->deleteLater();
    }
}

auto ConcurrentTask::getStepProgress() const -> TaskStepProgressList
{
    return m_task_progress.values();
}

void ConcurrentTask::addTask(Task::Ptr task)
{
    m_queue.append(task);
}

void ConcurrentTask::executeTask()
{
    // Start One task, startNext hadels starting the up to the m_total_max_size
    // while tracking the number currently being done
    QMetaObject::invokeMethod(this, &ConcurrentTask::startNext, Qt::QueuedConnection);
}

bool ConcurrentTask::abort()
{
    m_queue.clear();
    m_aborted = true;

    if (m_doing.isEmpty()) {
        // Don't call emitAborted() here, we want to bypass the 'is the task running' check
        emit aborted();
        emit finished();

        return true;
    }

    bool suceedeed = true;

    QMutableHashIterator<Task*, Task::Ptr> doing_iter(m_doing);
    while (doing_iter.hasNext()) {
        auto task = doing_iter.next();
        suceedeed &= (task.value())->abort();
    }

    if (suceedeed)
        emitAborted();
    else
        emitFailed(tr("Failed to abort all running tasks."));

    return suceedeed;
}

void ConcurrentTask::clear()
{
    Q_ASSERT(!isRunning());

    m_done.clear();
    m_failed.clear();
    m_queue.clear();

    m_aborted = false;

    m_progress = 0;
    m_stepProgress = 0;
}

void ConcurrentTask::startNext()
{
    if (m_aborted || m_doing.count() > m_total_max_size)
        return;

    if (m_queue.isEmpty() && m_doing.isEmpty() && !wasSuccessful()) {
        emitSucceeded();
        return;
    }

    if (m_queue.isEmpty())
        return;

    Task::Ptr next = m_queue.dequeue();

    connect(next.get(), &Task::succeeded, this, [this, next](){ subTaskSucceeded(next); });
    connect(next.get(), &Task::failed, this, [this, next](QString msg) { subTaskFailed(next, msg); });

    connect(next.get(), &Task::status, this, [this, next](QString msg){ subTaskStatus(next, msg); });
    connect(next.get(), &Task::details, this, [this, next](QString msg){ subTaskDetails(next, msg); });
    connect(next.get(), &Task::stepProgress, this, [this, next](TaskStepProgressList tp){ subTaskStepProgress(next, tp); });

    connect(next.get(), &Task::progress, this, [this, next](qint64 current, qint64 total){ subTaskProgress(next, current, total); });

    m_doing.insert(next.get(), next);
    m_task_progress.insert(next->getUid(), std::make_shared<TaskStepProgress>(TaskStepProgress({next->getUid()}))); 


    updateState();
    updateStepProgress();

    QCoreApplication::processEvents();

    QMetaObject::invokeMethod(next.get(), &Task::start, Qt::QueuedConnection);

    // Allow going up the number of concurrent tasks in case of tasks being added in the middle of a running task.
    int num_starts = qMin(m_queue.size(), m_total_max_size - m_doing.size());
    for (int i = 0; i < num_starts; i++)
        QMetaObject::invokeMethod(this, &ConcurrentTask::startNext, Qt::QueuedConnection);
}

void ConcurrentTask::subTaskSucceeded(Task::Ptr task)
{
    m_done.insert(task.get(), task);
    m_succeeded.insert(task.get(), task);

    m_doing.remove(task.get());
    m_task_progress.value(task->getUid())->state = TaskStepState::Succeeded;

    disconnect(task.get(), 0, this, 0);

    updateState();
    updateStepProgress();
    startNext();
}

void ConcurrentTask::subTaskFailed(Task::Ptr task, const QString& msg)
{
    m_done.insert(task.get(), task);
    m_failed.insert(task.get(), task);

    m_doing.remove(task.get());
    m_task_progress.value(task->getUid())->state = TaskStepState::Failed;

    disconnect(task.get(), 0, this, 0);

    updateState();
    updateStepProgress();
    startNext();
}

void ConcurrentTask::subTaskStatus(Task::Ptr task, const QString& msg)
{
    auto taskProgress = m_task_progress.value(task->getUid());
    taskProgress->status = msg; 
    taskProgress->state = TaskStepState::Running;
    updateStepProgress();
}

void ConcurrentTask::subTaskDetails(Task::Ptr task, const QString& msg)
{
    auto taskProgress = m_task_progress.value(task->getUid());
    taskProgress->details = msg; 
    taskProgress->state = TaskStepState::Running;
    updateStepProgress();
}

void ConcurrentTask::subTaskProgress(Task::Ptr task, qint64 current, qint64 total)
{
    auto taskProgress = m_task_progress.value(task->getUid());
    
    taskProgress->current = current;
    taskProgress->total = total;
    taskProgress->state = TaskStepState::Running;

    updateStepProgress();
    updateState();
}

void ConcurrentTask::subTaskStepProgress(Task::Ptr task, TaskStepProgressList task_step_progress)
{
    for (auto progress : task_step_progress) {
        if (!m_task_progress.contains(progress->uid)) {
            m_task_progress.insert(progress->uid, progress);
        } else {
            auto tp = m_task_progress.value(progress->uid);
            tp->current = progress->current;
            tp->total = progress->total;
            tp->status = progress->status;
            tp->details = progress->details;
        }           
    }

    updateStepProgress();
    
}

void ConcurrentTask::updateStepProgress()
{
   qint64 current = 0, total = 0;
   for ( auto taskProgress : m_task_progress ) {
       current += taskProgress->current;
       total += taskProgress->total;
   }

   m_stepProgress = current;
   m_stepTotalProgress = total;
   emit stepProgress(m_task_progress.values());
}

void ConcurrentTask::updateState()
{
    if (totalSize() > 1) {
        setProgress(m_done.count(), totalSize());
        setStatus(tr("Executing %1 task(s) (%2 out of %3 are done)").arg(QString::number(m_doing.count()), QString::number(m_done.count()), QString::number(totalSize())));
    } else {
        setProgress(m_stepProgress, m_stepTotalProgress);
        QString status = tr("Please wait ...");
        if (m_queue.size() > 0) {
            status = tr("Waiting for 1 task to start ...");
        } else if (m_doing.size() > 0) {
            status = tr("Executing 1 task:");
        } else if (m_done.size() > 0) {
            status = tr("Task finished.");
        }
        setStatus(status);
    }
}
