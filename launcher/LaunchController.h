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

#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/launch/MinecraftTarget.h"

class InstanceWindow;

enum class LaunchDecision { Undecided, Continue, Abort };

class LaunchController : public Task {
    Q_OBJECT
   public:
    void executeTask() override;

    LaunchController();
    ~LaunchController() override = default;

    void setInstance(BaseInstance* instance) { m_instance = instance; }

    BaseInstance* instance() const { return m_instance; }

    void setLaunchMode(const LaunchMode mode) { m_wantedLaunchMode = mode; }

    void setOfflineName(const QString& offlineName) { m_offlineName = offlineName; }

    void setProfiler(BaseProfilerFactory* profiler) { m_profiler = profiler; }

    void setParentWidget(QWidget* widget) { m_parentWidget = widget; }

    void setTargetToJoin(MinecraftTarget::Ptr targetToJoin) { m_targetToJoin = std::move(targetToJoin); }

    void setAccountToUse(MinecraftAccountPtr accountToUse) { m_accountToUse = std::move(accountToUse); }

    QString id() const { return m_instance->id(); }

    bool abort() override;

   private:
    void login();
    void launchInstance();
    void decideAccount();
    LaunchDecision decideLaunchMode();
    bool askPlayDemo() const;
    QString askOfflineName(const QString& playerName, bool* ok = nullptr) const;
    bool reauthenticateAccount(const MinecraftAccountPtr& account, const QString& reason);

   private slots:
    void readyForLaunch();

    void onSucceeded();
    void onFailed(QString reason);
    void onProgressRequested(Task* task) const;

   private:
    LaunchMode m_wantedLaunchMode = LaunchMode::Normal;
    LaunchMode m_actualLaunchMode = LaunchMode::Normal;
    BaseProfilerFactory* m_profiler = nullptr;
    QString m_offlineName;
    BaseInstance* m_instance = nullptr;
    QWidget* m_parentWidget = nullptr;
    InstanceWindow* m_console = nullptr;
    MinecraftAccountPtr m_accountToUse = nullptr;
    AuthSessionPtr m_session = nullptr;
    LaunchTask* m_launcher = nullptr;
    MinecraftTarget::Ptr m_targetToJoin = nullptr;
};
