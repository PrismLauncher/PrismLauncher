// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Colby Frison <colbyfrison100@gmail.com>
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
 */

#pragma once

#include <QString>

#include "minecraft/auth/AuthSession.h"
#include "minecraft/launch/MinecraftTarget.h"

class LaunchTask;

// Assembles an ordered LaunchStep list onto an existing LaunchTask.
// The caller still owns runtime refresh, icon save, join-target resolution, censor filters, and task ownership.
class LaunchPipelineBuilder {
   public:
    static constexpr auto StandardMinecraftRecipe = "standard-minecraft";

    explicit LaunchPipelineBuilder(LaunchTask* task);

    LaunchPipelineBuilder& setSession(AuthSessionPtr session);
    LaunchPipelineBuilder& setTargetToJoin(MinecraftTarget::Ptr target);
    LaunchPipelineBuilder& setPreLoadCommand(const QString& command);
    LaunchPipelineBuilder& setPreLaunchCommand(const QString& command);
    LaunchPipelineBuilder& setPostExitCommand(const QString& command);
    LaunchPipelineBuilder& setQuitAfterGameStop(bool quit);

    // Appends the named recipe. An unknown name appends nothing and returns false.
    bool apply(const QString& recipeName);

   private:
    void appendPreMetadata();
    void appendPreUpdate();
    void appendPreLaunch();
    void appendGame();
    void appendPostExit();

    LaunchTask* m_task;
    AuthSessionPtr m_session;
    MinecraftTarget::Ptr m_targetToJoin;
    QString m_preLoadCommand;
    QString m_preLaunchCommand;
    QString m_postExitCommand;
    bool m_quitAfterGameStop = false;
};
