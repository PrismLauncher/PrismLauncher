// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include "tasks/Task.h"

ConcurrentTask::ConcurrentTask(QObject* parent, QString task_name, int max_concurrent) : TaskV2(parent), m_total_max_size(max_concurrent)
{
    setObjectName(task_name);
    setCapabilities(Capability::Killable | Capability::Suspendable);
}

ConcurrentTask::~ConcurrentTask()
{
    for (auto task : m_doing) {
        if (task)
            task->disconnect(this);
    }
}

void ConcurrentTask::addTask(TaskV2::Ptr task)
{
    m_queue.append(task);
    task->setParent(this);
    setProgressTotal(progressTotal() + task->progressTotal() * task->weight());
    emit subTaskAdded(this, task.get());
}

void ConcurrentTask::executeTask()
{
    for (auto i = 0; i < m_total_max_size; i++)
        QMetaObject::invokeMethod(this, &ConcurrentTask::executeNextSubTask, Qt::QueuedConnection);
}

bool ConcurrentTask::doAbort()
{
    m_queue.clear();

    if (m_doing.isEmpty()) {
        return true;
    }

    bool suceedeed = true;

    QMutableHashIterator<QUuid, TaskV2::Ptr> doing_iter(m_doing);
    while (doing_iter.hasNext()) {
        auto task = doing_iter.next().value();
        suceedeed &= task->abort();
    }

    return suceedeed;
}

void ConcurrentTask::clear()
{
    Q_ASSERT(!isRunning());

    m_done.clear();
    m_failed.clear();
    m_queue.clear();

    setProgress(0);
}

void ConcurrentTask::executeNextSubTask()
{
    if (!isRunning()) {
        return;
    }
    if (m_doing.count() >= m_total_max_size) {
        return;
    }
    if (m_queue.isEmpty()) {
        if (m_doing.isEmpty()) {
            if (m_failed.isEmpty())
                emitSucceeded();
            else
                emitFailed(tr("One or more subtasks failed"));
        }
        return;
    }

    startSubTask(m_queue.dequeue());
}

void ConcurrentTask::startSubTask(TaskV2::Ptr next)
{
    connect(next.get(), &TaskV2::finished, this, &ConcurrentTask::subTaskFinished);

    connect(next.get(), &TaskV2::processedChanged, this, &ConcurrentTask::propateProcessedChanged);
    connect(next.get(), &TaskV2::totalChanged, this, &ConcurrentTask::propateTotalChanged);
    if (totalSize() == 1) {
        connect(next.get(), &TaskV2::stateChanged, this, &ConcurrentTask::propateState);
    }

    m_doing.insert(next->uuid(), next);

    updateState();

    next->start();
}

void ConcurrentTask::subTaskFinished(TaskV2* task)
{
    auto uuid = task->uuid();
    auto t = m_doing.value(uuid);
    m_done.insert(uuid, t);
    if (auto wasSuccesfull = t->state() & TaskV2::Succeeded) {
        m_succeeded.insert(uuid, t);
    } else {
        m_failed.insert(uuid, t);
    }

    m_doing.remove(uuid);

    disconnect(task, 0, this, 0);
    addWarnings(task->warnings());

    updateState();
    executeNextSubTask();
}

void ConcurrentTask::updateState()
{
    if (totalSize() > 1) {
        setStatus(tr("Executing %1 task(s) (%2 out of %3 are done)")
                      .arg(QString::number(m_doing.count()), QString::number(m_done.count()), QString::number(totalSize())));
    } else {
        QString status = tr("Please wait...");
        if (m_queue.size() > 0) {
            status = tr("Waiting for a task to start...");
        } else if (m_doing.size() > 0) {
            status = tr("Executing 1 task:");
        } else if (m_done.size() > 0) {
            status = tr("Task finished.");
        }
        setStatus(status);
    }
}

bool ConcurrentTask::doPause()
{
    for (auto task : m_doing) {
        task->pause();
    }
    return true;
}

bool ConcurrentTask::doResume()
{
    auto moreThreads = m_total_max_size - m_doing.size();
    for (auto task : m_doing) {
        task->resume();
    }
    for (auto i = 0; i < moreThreads; i++) {
        QMetaObject::invokeMethod(this, &ConcurrentTask::executeNextSubTask, Qt::QueuedConnection);
    }
    return true;
}
