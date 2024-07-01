// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLauncher - Minecraft Launcher
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
 */

#include "tasks/TaskManager.h"

bool TaskManager::addTask(Task::Ptr task, const QString& instanceId)
{
    auto instanceList = m_instanceTaskMap.find(instanceId);
    if (instanceList != m_instanceTaskMap.end()) {
        // don't add the same task more than once
        auto taskIter = std::find_if(instanceList->cbegin(), instanceList->cend(),
                                     [task](const Task::Ptr& instTask) { return instTask->getUid() == task->getUid(); });
        if (taskIter == instanceList->cend()) {
            instanceList->append(task);
            m_taskInstanceMap[task->getUid()] = instanceId;
            connectTask(task);
            return true;
        }
    } else {
        m_instanceTaskMap[instanceId].append(task);
        m_taskInstanceMap[task->getUid()] = instanceId;
        connectTask(task);
        return true;
    }
    return false;
}

bool TaskManager::removeTask(Task::Ptr task)
{
    auto iter = m_taskInstanceMap.find(task->getUid());
    if (iter != m_taskInstanceMap.end()) {
        QString instanceId = *iter;
        int i = 0;
        for (auto instTask : m_instanceTaskMap[instanceId]) {
            if (instTask->getUid() == task->getUid()) {
                m_instanceTaskMap[instanceId].removeAt(i);
                break;
            }
            i++;
        }
        m_taskInstanceMap.remove(task->getUid());
        disconnectTask(task);
        return true;
    }
    return false;
}

void TaskManager::execTask(Task::Ptr task, const QString& instanceId)
{
    addTask(task, instanceId);
    if (!task->isRunning()) {
        QMetaObject::invokeMethod(task.get(), &Task::start, Qt::QueuedConnection);
    } else {
        taskProgress(task->getUid(), task->getProgress(), task->getTotalProgress());
    }
}

QList<Task::Ptr> TaskManager::getAllTasks()
{
    QList<Task::Ptr> tasks;
    for (auto instanceList : m_instanceTaskMap) {
        tasks.append(instanceList);
    }
    return tasks;
}

QList<Task::Ptr> TaskManager::getTasks(const QString& instanceId)
{
    auto instanceList = m_instanceTaskMap.find(instanceId);
    if (instanceList != m_instanceTaskMap.end()) {
        return *instanceList;
    } else {
        return {};
    }
}

Task::Ptr TaskManager::getTask(QUuid taskId)
{
    auto taskInstanceIter = m_taskInstanceMap.find(taskId);
    if (taskInstanceIter != m_taskInstanceMap.end()) {
        auto taskListIter = m_instanceTaskMap.find(*taskInstanceIter);
        if (taskListIter != m_instanceTaskMap.end()) {
            auto taskList = *taskListIter;
            auto taskIter = std::find_if(taskList.begin(), taskList.end(),
                                         [taskId](const Task::Ptr& instTask) { return instTask->getUid() == taskId; });
            if (taskIter != taskList.end()) {
                return *taskIter;
            }
        }
    }
    return nullptr;
}

QString TaskManager::getTaskInstanceId(QUuid taskId)
{
    auto taskInstanceIter = m_taskInstanceMap.find(taskId);
    if (taskInstanceIter != m_taskInstanceMap.end()) {
        return *taskInstanceIter;
    }
    return "";
}

std::pair<qint64, qint64> TaskManager::getAllProgress()
{
    qint64 progress = 0;
    qint64 total = 0;
    for (auto instanceList : m_instanceTaskMap) {
        for (auto task : instanceList) {
            total += task->getProgress();
            progress += task->getTotalProgress();
        }
    }
    return { progress, total };
}

std::pair<qint64, qint64> TaskManager::getProgress(const QString& instanceId)
{
    qint64 progress = 0;
    qint64 total = 0;
    auto instanceList = m_instanceTaskMap.find(instanceId);
    if (instanceList != m_instanceTaskMap.end()) {
        for (auto task : *instanceList) {
            total += task->getProgress();
            progress += task->getTotalProgress();
        }
    }
    return { progress, total };
}

void TaskManager::connectTask(Task::Ptr task)
{
    QUuid taskId = task->getUid();
    m_taskConnectionMap[taskId] << connect(task.get(), &Task::finished, this, [this, taskId]() { taskFinished(taskId); });
    m_taskConnectionMap[taskId] << connect(task.get(), &Task::progress, this,
                                           [this, taskId](qint64 current, qint64 total) { taskProgress(taskId, current, total); });
}
void TaskManager::disconnectTask(Task::Ptr task)
{
    auto connectionList = m_taskConnectionMap.find(task->getUid());
    if (connectionList != m_taskConnectionMap.end()) {
        for (auto connection : *connectionList) {
            QObject::disconnect(connection);
        }
    }
}

void TaskManager::taskFinished(QUuid taskId)
{
    Task::Ptr task = getTask(taskId);
    if (task) {
        QString instanceId = getTaskInstanceId(taskId);
        emit finished(taskId, instanceId);
        removeTask(task);
    }
}

void TaskManager::taskProgress(QUuid taskId, qint64 current, qint64 total)
{
    Task::Ptr task = getTask(taskId);
    if (task) {
        QString instanceId = getTaskInstanceId(taskId);
        emit progress(taskId, instanceId, current, total);
    }
}