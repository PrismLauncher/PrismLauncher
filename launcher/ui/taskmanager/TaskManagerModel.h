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

#include <QAbstractItemModel>
#include <memory>
#include <variant>
#include "Application.h"
#include "tasks/Task.h"
#include "tasks/TaskManager.h"

class TaskTreeItem {
   public:
    enum class ItemType { Task, Progress };

   public:
    explicit TaskTreeItem(Task* task, TaskTreeItem* parent = nullptr);
    explicit TaskTreeItem(TaskStepProgress* taskProgress, TaskTreeItem* parent = nullptr);

    TaskTreeItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    int row(QUuid subtaskId) const;
    TaskTreeItem* parentItem();

   private:
    QMap<QUuid, std::unique_ptr<TaskTreeItem>> m_childItems;
    TaskStepProgress* m_itemProgress = nullptr;
    Task* m_itemTask = nullptr;
    ItemType m_itemType;
    TaskTreeItem* m_parent;

   private:
    TaskTreeItem* getProgressChild(TaskStepProgress* taskProgress);
    TaskManager* taskManager() const { return APPLICATION->taskManager(); }
};

class TaskManagerModel : public QAbstractItemModel {
    Q_OBJECT

   public:
    Q_DISABLE_COPY_MOVE(TaskManagerModel)
    enum Column { Name, Id, State, Status, Details, Progress, Weight };

    explicit TaskManagerModel();
    ~TaskManagerModel() override = default;

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
   
   public slots:
    void taskAdded(QUuid taskId);
    void taskRemoved(QUuid taskId);
    void taskStateChanged(QUuid taskId);
    void subtaskStateChanged(QUuid taskId, QUuid subtaskId);

   private:
    TaskManager* taskManager() const { return APPLICATION->taskManager(); }
    void setupTree();
    QMap<QUuid, std::unique_ptr<TaskTreeItem>> m_treeItems;
   private:
};
