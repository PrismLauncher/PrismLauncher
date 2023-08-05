/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QList>
#include <QObject>
#include <QUrl>

#include <quazip/quazip.h>
#include "minecraft/VersionFilterData.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class MinecraftVersion;
class MinecraftInstance;

// FIXME: This looks very similar to a SequentialTask. Maybe we can reduce code duplications? :^)

class MinecraftUpdate : public Task {
    Q_OBJECT
   public:
    explicit MinecraftUpdate(MinecraftInstance* inst, QObject* parent = 0);
    virtual ~MinecraftUpdate(){};

    void executeTask() override;
    bool canAbort() const override;

   private slots:
    bool abort() override;
    void subtaskSucceeded();
    void subtaskFailed(QString error);

   private:
    void next();

   private:
    MinecraftInstance* m_inst = nullptr;
    QList<Task::Ptr> m_tasks;
    QString m_preFailure;
    int m_currentTask = -1;
    bool m_abort = false;
    bool m_failed_out_of_order = false;
    QString m_fail_reason;
};
