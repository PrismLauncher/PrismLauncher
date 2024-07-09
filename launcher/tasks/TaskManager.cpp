// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLauncher - Minecraft Launcher
 *  Copyright (c) 2024 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
    auto taskId = task->getUid();
    auto instanceList = m_instanceTaskMap.find(instanceId);
    if (instanceList != m_instanceTaskMap.end()) {
        // don't add the same task more than once
        auto taskIter = std::find_if(instanceList->cbegin(), instanceList->cend(),
                                     [task](const Task::Ptr& instTask) { return instTask->getUid() == task->getUid(); });
        if (taskIter == instanceList->cend()) {
            instanceList->append(task);
            m_taskInstanceMap[taskId] = instanceId;
            connectTask(task);
            emit taskAdded(taskId);
            return true;
        }
    } else {
        m_instanceTaskMap[instanceId].append(task);
        m_taskInstanceMap[taskId] = instanceId;
        connectTask(task);
        emit taskAdded(taskId);
        return true;
    }
    return false;
}

bool TaskManager::removeTask(Task::Ptr task)
{
    auto taskId = task->getUid();
    auto iter = m_taskInstanceMap.find(taskId);
    if (iter != m_taskInstanceMap.end()) {
        QString instanceId = *iter;
        int i = 0;
        for (auto instTask : m_instanceTaskMap[instanceId]) {
            if (instTask->getUid() == taskId) {
                m_instanceTaskMap[instanceId].removeAt(i);
                break;
            }
            i++;
        }
        m_taskInstanceMap.remove(taskId);
        disconnectTask(task);
        emit taskRemoved(taskId);
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
            auto taskList = taskListIter.value();
            auto taskIter = std::find_if(taskList.begin(), taskList.end(),
                                         [taskId](const Task::Ptr& instTask) { return instTask->getUid() == taskId; });
            if (taskIter != taskList.end()) {
                return *taskIter;
            }
        }
    }
    return nullptr;
}

Task::Ptr TaskManager::getTask(int index)
{
    int count = index;
    auto it = m_taskInstanceMap.cbegin();
    while (it != m_taskInstanceMap.cend() && count > 0) {
        it++;
        count--;
    }
    if (count == 0 && it != m_taskInstanceMap.cend()) {
        auto taskId = it.key();
        auto taskInstanceIter = m_instanceTaskMap.find(it.value());
        if (taskInstanceIter != m_instanceTaskMap.end()) {
            auto taskList = taskInstanceIter.value();
            auto taskIter = std::find_if(taskList.begin(), taskList.end(),
                                         [taskId](const Task::Ptr& instTask) { return instTask->getUid() == taskId; });
            if (taskIter != taskList.end()) {
                return *taskIter;
            }
        }
    }
    return nullptr;
}

int TaskManager::getTaskIndex(QUuid taskId)
{
    QMap<QUuid, QString>::const_iterator it = m_taskInstanceMap.find(taskId);
    if (it != m_taskInstanceMap.cend()) {
        return std::distance(m_taskInstanceMap.cbegin(), it);
    }
    return -1;
}

QString TaskManager::getTaskInstanceId(QUuid taskId)
{
    auto taskInstanceIter = m_taskInstanceMap.find(taskId);
    if (taskInstanceIter != m_taskInstanceMap.end()) {
        return *taskInstanceIter;
    }
    return "";
}

std::pair<double, double> TaskManager::getAllProgress()
{
    double progress = 0;
    double total = 0;
    for (auto instanceList : m_instanceTaskMap) {
        for (auto task : instanceList) {
            total += task->getProgress();
            progress += task->getTotalProgress();
        }
    }
    return { progress, total };
}

std::pair<double, double> TaskManager::getProgress(const QString& instanceId)
{
    double progress = 0;
    double total = 0;
    auto instanceList = m_instanceTaskMap.find(instanceId);
    if (instanceList != m_instanceTaskMap.end()) {
        for (auto task : *instanceList) {
            progress += task->getProgress();
            total += task->getTotalProgress();
        }
    }
    return { progress, total };
}

void TaskManager::connectTask(Task::Ptr task)
{
    QUuid taskId = task->getUid();
    m_taskConnectionMap[taskId] << connect(task.get(), &Task::finished, this, [this, taskId]() { taskFinished(taskId); });
    m_taskConnectionMap[taskId] << connect(task.get(), &Task::progress, this,
                                           [this, taskId](double current, double total) { taskProgress(taskId, current, total); });
    m_taskConnectionMap[taskId] << connect(task.get(), &Task::stepProgress, this, [this, taskId](TaskStepProgress const& task_progress) {
        subtaskProgress(taskId, task_progress);
    });
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

void TaskManager::taskProgress(QUuid taskId, double current, double total)
{
    Task::Ptr task = getTask(taskId);
    if (task) {
        QString instanceId = getTaskInstanceId(taskId);
        emit taskStateChanged(taskId, instanceId, current, total, task->getStatus(), task->getDetails(), task->getState());
    }
}

void TaskManager::subtaskProgress(QUuid taskId, TaskStepProgress const& task_progress)
{
    Task::Ptr task = getTask(taskId);
    if (task) {
        QString instanceId = getTaskInstanceId(taskId);
        emit subtaskStateChanged(taskId, instanceId, task_progress);
    }
}
