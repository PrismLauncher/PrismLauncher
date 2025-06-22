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

#include <LoggedProcess.h>
#include <QObjectPtr.h>
#include <java/JavaChecker.h>
#include <launch/LaunchStep.h>
#include <net/Mode.h>

class TaskStepWrapper : public LaunchStep {
    Q_OBJECT
   public:
    explicit TaskStepWrapper(LaunchTask* parent, Task::Ptr task) : LaunchStep(parent), m_task(task) {};
    virtual ~TaskStepWrapper() = default;

    void executeTask() override;
    bool canAbort() const override;
    void proceed() override;
   public slots:
    bool abort() override;

   private slots:
    void updateFinished();

   private:
    Task::Ptr m_task;
};
