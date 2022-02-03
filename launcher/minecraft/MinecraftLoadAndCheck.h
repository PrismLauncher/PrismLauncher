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

#include <QObject>
#include <QList>
#include <QUrl>

#include "tasks/Task.h"
#include <quazip/quazip.h>

#include "QObjectPtr.h"

class MinecraftVersion;
class MinecraftInstance;

class MinecraftLoadAndCheck : public Task
{
    Q_OBJECT
public:
    explicit MinecraftLoadAndCheck(MinecraftInstance *inst, QObject *parent = 0);
    virtual ~MinecraftLoadAndCheck() {};
    void executeTask() override;

private slots:
    void subtaskSucceeded();
    void subtaskFailed(QString error);

private:
    MinecraftInstance *m_inst = nullptr;
    Task::Ptr m_task;
    QString m_preFailure;
    QString m_fail_reason;
};

