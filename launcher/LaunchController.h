// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
#include <QObject>
#include <BaseInstance.h>
#include <tools/BaseProfiler.h>

#include "minecraft/launch/MinecraftServerTarget.h"
#include "minecraft/auth/MinecraftAccount.h"

class InstanceWindow;
class LaunchController: public Task
{
    Q_OBJECT
public:
    void executeTask() override;

    LaunchController(QObject * parent = nullptr);
    virtual ~LaunchController(){};

    void setInstance(InstancePtr instance) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance = instance;
    }

    InstancePtr instance() {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance;
    }

    void setOnline(bool online) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_online = online;
    }

    void setDemo(bool demo) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_demo = demo;
    }

    void setProfiler(BaseProfilerFactory *profiler) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profiler = profiler;
    }

    void setParentWidget(QWidget * widget) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parentWidget = widget;
    }

    void setServerToJoin(MinecraftServerTargetPtr serverToJoin) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin = std::move(serverToJoin);
    }

    void setAccountToUse(MinecraftAccountPtr accountToUse) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_accountToUse = std::move(accountToUse);
    }

    QString id()
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->id();
    }

    bool abort() override;

private:
    void login();
    void launchInstance();
    void decideAccount();

private slots:
    void readyForLaunch();

    void onSucceeded();
    void onFailed(QString reason);
    void onProgressRequested(Task *task);

private:
    BaseProfilerFactory *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profiler = nullptr;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_online = true;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_demo = false;
    InstancePtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance;
    QWidget * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parentWidget = nullptr;
    InstanceWindow *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_console = nullptr;
    MinecraftAccountPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_accountToUse = nullptr;
    AuthSessionPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_session;
    shared_qobject_ptr<LaunchTask> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launcher;
    MinecraftServerTargetPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin;
};
