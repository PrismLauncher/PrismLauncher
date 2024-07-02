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
#pragma once

#include <QHash>
#include <QQueue>
#include <QSet>
#include <QUuid>
#include <memory>

#include "tasks/Task.h"

class ConcurrentTask : public Task {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<ConcurrentTask>;

    explicit ConcurrentTask(QObject* parent = nullptr, QString task_name = "", int max_concurrent = 6);
    ~ConcurrentTask() override;

    // safe to call before starting the task
    void setMaxConcurrent(int max_concurrent) { m_total_max_size = max_concurrent; }

    bool canAbort() const override { return true; }

    inline auto isMultiStep() const -> bool override { return totalSize() > 1; }
    TaskStepProgressList getStepProgress() const override;

    void addTask(Task::Ptr task, double weight = 1);

   public slots:
    bool abort() override;

    /** Resets the internal state of the task.
     *  This allows the same task to be re-used.
     */
    void clear();

   protected slots:
    void executeTask() override;

    virtual void executeNextSubTask();

    void subTaskSucceeded(Task::Ptr);
    virtual void subTaskFailed(Task::Ptr, const QString& msg);
    void subTaskFinished(Task::Ptr, TaskStepState);
    void subTaskStatus(Task::Ptr task, const QString& msg);
    void subTaskDetails(Task::Ptr task, const QString& msg);
    void subTaskProgress(Task::Ptr task, double current, double total);
    void subTaskStepProgress(Task::Ptr task, TaskStepProgress const& task_step_progress);

   protected:
    // NOTE: This is not thread-safe.
    [[nodiscard]] unsigned int totalSize() const { return static_cast<unsigned int>(m_queue.size() + m_doing.size() + m_done.size()); }

    enum class Operation { ADDED, REMOVED, CHANGED };
    void updateStepProgress(TaskStepProgress const& changed_progress, Operation);

    virtual void updateState();

    void startSubTask(Task::Ptr task, double weight);

   protected:
    QString m_name;
    QString m_step_status;

    // queue of task, weight pairs
    QQueue<std::pair<Task::Ptr, double>> m_queue;

    QHash<Task*, std::pair<Task::Ptr, double>> m_doing;
    QHash<Task*, std::pair<Task::Ptr, double>> m_done;
    QHash<Task*, std::pair<Task::Ptr, double>> m_failed;
    QHash<Task*, std::pair<Task::Ptr, double>> m_succeeded;

    QHash<QUuid, std::shared_ptr<TaskStepProgress>> m_task_progress;

    int m_total_max_size;

    qint64 m_stepProgress = 0;
    qint64 m_stepTotalProgress = 100;
};
