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

#include "net/Mode.h"
#include "tasks/Task.h"

class MinecraftInstance;

class MinecraftLoadAndCheck : public Task {
    Q_OBJECT
   public:
    explicit MinecraftLoadAndCheck(MinecraftInstance* inst, Net::Mode netmode);
    virtual ~MinecraftLoadAndCheck() = default;
    void executeTask() override;

    bool canAbort() const override;
   public slots:
    bool abort() override;

   private:
    MinecraftInstance* m_inst = nullptr;
    Task::Ptr m_task;
    Net::Mode m_netmode;
};
