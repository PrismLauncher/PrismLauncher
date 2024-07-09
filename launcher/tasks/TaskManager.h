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

#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QUuid>

#include "tasks/Task.h"

class TaskManager : public QObject {
    Q_OBJECT
   public:
    TaskManager() = default;

    bool addTask(Task::Ptr task, const QString& instanceId = "");
    bool removeTask(Task::Ptr task);
    void execTask(Task::Ptr task, const QString& instanceId = "");

    QList<Task::Ptr> getAllTasks();
    QList<Task::Ptr> getTasks(const QString& instanceId = "");

    Task::Ptr getTask(QUuid taskId);
    Task::Ptr getTask(int index);
    int getTaskIndex(QUuid taskId);
    QString getTaskInstanceId(QUuid taskId);

    std::pair<double, double> getAllProgress();
    std::pair<double, double> getProgress(const QString& instanceId = "");

   private:
    void connectTask(Task::Ptr task);
    void disconnectTask(Task::Ptr task);

    QMap<QString, QList<Task::Ptr>> m_instanceTaskMap;
    QMap<QUuid, QString> m_taskInstanceMap;
    QMap<QUuid, QList<QMetaObject::Connection>> m_taskConnectionMap;

   signals:
    void taskAdded(QUuid taskId);
    void taskRemoved(QUuid taskId);
    void
    taskStateChanged(QUuid taskId, QString instanceId, double current, double total, QString status, QString details, Task::State state);
    void subtaskStateChanged(QUuid taskId, QString instanceId, TaskStepProgress const& task_progress);
    void finished(QUuid taskId, QString instanceId);

   public slots:
    void taskFinished(QUuid taskId);
    void taskProgress(QUuid taskId, double current, double total);
    void subtaskProgress(QUuid taskId, TaskStepProgress const& task_progress);
};
