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
    QString getTaskInstanceId(QUuid taskId);

    std::pair<qint64, qint64> getAllProgress();
    std::pair<qint64, qint64> getProgress(const QString& instanceId = "");

   private:
    void connectTask(Task::Ptr task);
    void disconnectTask(Task::Ptr task);

    QMap<QString, QList<Task::Ptr>> m_instanceTaskMap;
    QMap<QUuid, QString> m_taskInstanceMap;
    QMap<QUuid, QList<QMetaObject::Connection>> m_taskConnectionMap;

   signals:
    void progress(QUuid taskId, QString instanceId, qint64 current, qint64 total);
    void finished(QUuid taskId, QString instanceId);

   public slots:
    void taskFinished(QUuid taskId);
    void taskProgress(QUuid taskId, qint64 current, qint64 total);
};
