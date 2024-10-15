// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
#include <BaseInstance.h>
#include <tools/BaseProfiler.h>
#include <QObject>

#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/launch/MinecraftTarget.h"

class InstanceWindow;
class LaunchController : public Task {
    Q_OBJECT
   public:
    void executeTask() override;

    LaunchController(QObject* parent = nullptr);
    virtual ~LaunchController() = default;

    void setInstance(InstancePtr instance) { m_instance = instance; }

    InstancePtr instance() { return m_instance; }

    void setOnline(bool online) { m_online = online; }

    void setDemo(bool demo) { m_demo = demo; }

    void setAskServerAddress(bool askServerAddress) { m_askServerAddress = askServerAddress; }

    void setProfiler(BaseProfilerFactory* profiler) { m_profiler = profiler; }

    void setParentWidget(QWidget* widget) { m_parentWidget = widget; }

    void setTargetToJoin(MinecraftTarget::Ptr targetToJoin) { m_targetToJoin = std::move(targetToJoin); }

    void setAccountToUse(MinecraftAccountPtr accountToUse) { m_accountToUse = std::move(accountToUse); }

    QString id() { return m_instance->id(); }

    bool abort() override;

   private:
    void login();
    void launchInstance();
    void decideAccount();
    bool askPlayDemo();
    QString askOfflineName(QString playerName, bool demo, bool* ok);
    QString askServerAddress(bool* ok);

   private slots:
    void readyForLaunch();

    void onSucceeded();
    void onFailed(QString reason);
    void onProgressRequested(Task* task);

   private:
    BaseProfilerFactory* m_profiler = nullptr;
    bool m_online = true;
    bool m_demo = false;
    bool m_askServerAddress = false;
    InstancePtr m_instance;
    QWidget* m_parentWidget = nullptr;
    InstanceWindow* m_console = nullptr;
    MinecraftAccountPtr m_accountToUse = nullptr;
    AuthSessionPtr m_session;
    shared_qobject_ptr<LaunchTask> m_launcher;
    MinecraftTarget::Ptr m_targetToJoin;
};
