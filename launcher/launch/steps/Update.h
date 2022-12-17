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

#include <launch/LaunchStep.h>
#include <QObjectPtr.h>
#include <LoggedProcess.h>
#include <java/JavaChecker.h>
#include <net/Mode.h>

// FIXME: stupid. should be defined by the instance type? or even completely abstracted away...
class Update: public LaunchStep
{
    Q_OBJECT
public:
    explicit Update(LaunchTask *parent, Net::Mode mode):LaunchStep(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mode(mode) {};
    virtual ~Update() {};

    void executeTask() override;
    bool canAbort() const override;
    void proceed() override;
public slots:
    bool abort() override;

private slots:
    void updateFinished();

private:
    Task::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = false;
    Net::Mode hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mode = Net::Mode::Offline;
};
