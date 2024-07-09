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
#pragma once

#include <QHash>
#include <QList>
#include <QQueue>
#include <QSet>
#include <QUuid>

#include "tasks/Task.h"

class ConcurrentTask : public TaskV2 {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<ConcurrentTask>;

    explicit ConcurrentTask(QObject* parent = nullptr, QString task_name = "", int max_concurrent = 6);
    ~ConcurrentTask() override;

    // safe to call before starting the task
    void setMaxConcurrent(int max_concurrent) { m_total_max_size = max_concurrent; }

    inline bool isMultiStep() const override { return totalSize() > 1; }
    virtual QList<TaskV2::Ptr> subTasks() const override { return m_queue.toList() + m_doing.values() + m_done.values(); }

    void addTask(TaskV2::Ptr task);

    // NOTE: This is not thread-safe.
    [[nodiscard]] unsigned int totalSize() const { return static_cast<unsigned int>(m_queue.size() + m_doing.size() + m_done.size()); }

   public slots:

    /** Resets the internal state of the task.
     *  This allows the same task to be re-used.
     */
    void clear();

   protected slots:
    void executeTask() override;
    bool doAbort() override;
    bool doPause() override;
    bool doResume() override;

    virtual void executeNextSubTask();
    virtual void subTaskFinished(TaskV2*);

   protected:
    virtual void updateState();

    void startSubTask(TaskV2::Ptr task);

   protected:
    QString m_step_status;

    QQueue<TaskV2::Ptr> m_queue;

    QHash<QUuid, TaskV2::Ptr> m_doing;
    QHash<QUuid, TaskV2::Ptr> m_done;
    QHash<QUuid, TaskV2::Ptr> m_failed;
    QHash<QUuid, TaskV2::Ptr> m_succeeded;

    int m_total_max_size;
};
