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

// FIXME: stupid. should be defined by the instance type? or even completely abstracted away...
class Update : public LaunchStep {
    Q_OBJECT
   public:
    explicit Update(LaunchTask* parent, Net::Mode mode) : LaunchStep(parent), m_mode(mode){};
    virtual ~Update(){};

    void executeTask() override;
    bool canAbort() const override;
    void proceed() override;
   public slots:
    bool abort() override;

   private slots:
    void updateFinished();

   private:
    Task::Ptr m_updateTask;
    bool m_aborted = false;
    Net::Mode m_mode = Net::Mode::Offline;
};
