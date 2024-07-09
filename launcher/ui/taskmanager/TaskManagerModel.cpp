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

#include "ui/taskmanager/TaskManagerModel.h"
#include <quuid.h>
#include <algorithm>
#include <memory>
#include "tasks/Task.h"

TaskTreeItem::TaskTreeItem(Task* task, TaskTreeItem* parent) : m_itemTask(task), m_itemType(ItemType::Task), m_parent(parent) {}
TaskTreeItem::TaskTreeItem(TaskStepProgress* taskProgress, TaskTreeItem* parent)
    : m_itemProgress(taskProgress), m_itemType(ItemType::Progress), m_parent(parent)
{}

template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

#include <variant>
TaskTreeItem* TaskTreeItem::child(int row)
{
    if (row >= 0) {
        TaskTreeItem* child = nullptr;
        if (m_itemType == ItemType::Task && m_itemTask) {
            auto taskProgresses = m_itemTask->getStepProgress();
            if (row < taskProgresses.size()) {
                auto progress = taskProgresses.at(row);
                child = getProgressChild(progress.get());
            }
        }
        return child;
    } else {
        return nullptr;
    }
}

int TaskTreeItem::childCount() const
{
    if (m_itemType == ItemType::Task && m_itemTask) {
        auto taskProgresses = m_itemTask->getStepProgress();
        return taskProgresses.size();
    }
    return 0;
}

int TaskTreeItem::columnCount() const
{
    if (m_itemType == ItemType::Task && m_itemTask) {
        return 5;
    }
    if (m_itemType == ItemType::Progress && m_itemProgress) {
        return 5;
    }
    return 0;
}

QString stateToString(Task::State state)
{
    switch (state) {
        case Task::State::Inactive:
            return QObject::tr("Inactive");
        case Task::State::Running:
            return QObject::tr("Running");
        case Task::State::Succeeded:
            return QObject::tr("Succeeded");
        case Task::State::Failed:
            return QObject::tr("Failed");
        case Task::State::AbortedByUser:
            return QObject::tr("Aborted by User");
    }
    return "Unknown";  // impossible
}
QString stateToString(TaskStepState state)
{
    switch (state) {
        case TaskStepState::Waiting:
            return QObject::tr("Waiting");
        case TaskStepState::Running:
            return QObject::tr("Running");
        case TaskStepState::Succeeded:
            return QObject::tr("Succeeded");
        case TaskStepState::Failed:
            return QObject::tr("Failed");
    }
    return "Unknown";  // impossible
}

QVariant TaskTreeItem::data(int column) const
{
    if (m_itemType == ItemType::Task && m_itemTask) {
        switch (TaskManagerModel::Column(column)) {
            case TaskManagerModel::Column::Name: {
                auto name = m_itemTask->objectName();
                return name.isEmpty() ? QString("0x%1").arg(reinterpret_cast<quintptr>(m_itemTask)) : name;
            }
            case TaskManagerModel::Column::Id:
                return m_itemTask->getUid().toString();
            case TaskManagerModel::Column::State:
                return stateToString(m_itemTask->getState());
            case TaskManagerModel::Column::Status:
                return m_itemTask->getStatus();
            case TaskManagerModel::Column::Details:
                return m_itemTask->getDetails();
            case TaskManagerModel::Column::Progress:
                return m_itemTask->getTotalProgress() ? m_itemTask->getProgress() / m_itemTask->getTotalProgress() : -1;
            case TaskManagerModel::Column::Weight:
                return {};
        }
        return {};
    }
    if (m_itemType == ItemType::Progress && m_itemProgress) {
        switch (TaskManagerModel::Column(column)) {
            case TaskManagerModel::Column::Name:
                return {};
            case TaskManagerModel::Column::Id:
                return m_itemProgress->uid;
            case TaskManagerModel::Column::State:
                return stateToString(m_itemProgress->state);
            case TaskManagerModel::Column::Status:
                return m_itemProgress->status;
            case TaskManagerModel::Column::Details:
                return m_itemProgress->details;
            case TaskManagerModel::Column::Progress:
                return m_itemProgress->total > 0 ? m_itemProgress->current / m_itemProgress->total : -1;
            case TaskManagerModel::Column::Weight:
                return m_itemProgress->weight;
        }
        return {};
    }
    return {};
}

int TaskTreeItem::row() const
{
    if (m_parent == nullptr) {
        return 0;
    }
    if (m_itemType == ItemType::Progress && m_itemProgress) {
        if (m_parent->m_itemType == ItemType::Task && m_parent->m_itemTask) {
            auto taskProgressList = m_parent->m_itemTask->getStepProgress();
            const auto it =
                std::find_if(taskProgressList.cbegin(), taskProgressList.cend(),
                             [this](const std::shared_ptr<TaskStepProgress> progress) { return progress.get() == m_itemProgress; });
            if (it != taskProgressList.cend()) {
                return std::distance(taskProgressList.cbegin(), it);
            }
        }
    }
    return -1;
}

int TaskTreeItem::row(QUuid subtaskId) const
{
    if (m_itemType == ItemType::Task && m_itemTask) {
        auto taskProgressList = m_itemTask->getStepProgress();
        const auto it = std::find_if(taskProgressList.cbegin(), taskProgressList.cend(),
                                     [subtaskId](const std::shared_ptr<TaskStepProgress> progress) { return progress->uid == subtaskId; });
        if (it != taskProgressList.cend()) {
            return std::distance(taskProgressList.cbegin(), it);
        }
    }
    return -1;
}

TaskTreeItem* TaskTreeItem::parentItem()
{
    return m_parent;
}

TaskTreeItem* TaskTreeItem::getProgressChild(TaskStepProgress* taskProgress)
{
    auto childIter = m_childItems.find(taskProgress->uid);
    if (childIter != m_childItems.end()) {
        return childIter.value().get();
    } else {
        auto item = m_childItems.insert(taskProgress->uid, std::make_unique<TaskTreeItem>(taskProgress));
        return item.value().get();
    }
}

TaskManagerModel::TaskManagerModel()
{
    auto taskManager = APPLICATION->taskManager();
    connect(taskManager, &TaskManager::taskAdded, this, &TaskManagerModel::taskAdded);
    connect(taskManager, &TaskManager::taskRemoved, this, &TaskManagerModel::taskRemoved);
    connect(taskManager, &TaskManager::taskStateChanged, this,
            [this](QUuid taskId, QString, double, double, QString, QString, Task::State) { taskStateChanged(taskId); });
    connect(taskManager, &TaskManager::subtaskStateChanged, this,
            [this](QUuid taskId, TaskStepProgress const& task_progress) { subtaskStateChanged(taskId, task_progress.uid); });
}

void TaskManagerModel::setupTree()
{
    auto tasks = taskManager()->getTasks();
    for (auto task : tasks) {
        m_treeItems.insert(task->getUid(), std::make_unique<TaskTreeItem>(task.get()));
    }
}

void TaskManagerModel::taskAdded(QUuid taskId)
{
    auto it = m_treeItems.insert(taskId, std::make_unique<TaskTreeItem>(taskManager()->getTask(taskId).get()));
    auto i = int(std::distance(m_treeItems.begin(), it));
    auto last = int(m_treeItems.size()) - 1;
    emit dataChanged(index(i, 0), index(last, columnCount(index(last, 0)) - 1));
}

void TaskManagerModel::taskRemoved(QUuid taskId)
{
    if (m_treeItems.remove(taskId) > 0) {
        auto last = int(m_treeItems.size()) - 1;
        emit dataChanged(index(0, 0), index(last, columnCount(index(last, 0)) - 1));
    }
}

void TaskManagerModel::taskStateChanged(QUuid taskId)
{
    auto it = m_treeItems.find(taskId);
    if (it != m_treeItems.end()) {
        auto i = int(std::distance(m_treeItems.begin(), it));
        auto taskIndex = index(i, 0);
        emit dataChanged(taskIndex, index(i, columnCount(taskIndex) - 1));
    }
}

void TaskManagerModel::subtaskStateChanged(QUuid taskId, QUuid subtaskId)
{
    auto it = m_treeItems.find(taskId);
    if (it != m_treeItems.end()) {
        auto row = it.value()->row(subtaskId);
        auto taskIndex = index(row, 0);
        emit dataChanged(taskIndex, index(row, columnCount(taskIndex) - 1));
    }
}


QModelIndex TaskManagerModel::index(int row, int column, const QModelIndex& parent) const
{
    if (TaskTreeItem* parentItem = parent.isValid() ? static_cast<TaskTreeItem*>(parent.internalPointer()) : nullptr) {
        return createIndex(row, column, parentItem->child(row));
    } else {
        int count = row;
        auto it = m_treeItems.cbegin();
        while (it != m_treeItems.cend() && count > 0) {
            it++;
            count--;
        }
        if (count == 0 && it != m_treeItems.cend()) {
            auto taskItem = it.value().get();
            return createIndex(row, column, taskItem);
        }
        return {};
    }
}

QModelIndex TaskManagerModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return {};
    }

    auto* childItem = static_cast<TaskTreeItem*>(index.internalPointer());
    TaskTreeItem* parentItem = childItem->parentItem();
    if (parentItem) {
        return createIndex(parentItem->row(), 0, parentItem);
    }
    return {};
}

int TaskManagerModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    if (parent.isValid()) {
        const TaskTreeItem* parentItem = static_cast<const TaskTreeItem*>(parent.internalPointer());
        return parentItem->childCount();
    }
    return m_treeItems.size();
}

int TaskManagerModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return static_cast<const TaskTreeItem*>(parent.internalPointer())->columnCount();
    }
    return 2;
}

QVariant TaskManagerModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const TaskTreeItem* item = static_cast<const TaskTreeItem*>(index.internalPointer());
    if (role == Qt::DisplayRole) {
        return item->data(index.column());
    }
    return {};
}
