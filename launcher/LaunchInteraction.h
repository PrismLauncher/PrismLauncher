// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
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

#include <optional>

#include "minecraft/auth/MinecraftAccount.h"

class MinecraftInstance;
class Task;

/**
 * Every question the launch pipeline (LaunchController) may need to ask the user.
 *
 * Without an interaction object LaunchController shows its classic Qt dialogs. The web UI installs
 * an implementation (api/WebLaunchInteraction) that answers from the launch request and reports
 * problems to the frontend instead of blocking on modal dialogs.
 */
class LaunchInteraction {
   public:
    virtual ~LaunchInteraction() = default;

    /** No account is set as default but some are valid. Return the one to use, or nullptr. */
    virtual MinecraftAccountPtr chooseAccount(MinecraftInstance* instance) = 0;

    /** Blocks until `task` (e.g. an account refresh) finishes. Returns false if the user aborted it. */
    virtual bool waitForTask(MinecraftInstance* instance, Task* task, const QString& title) = 0;

    /** Account refresh failed for `reason`. Return true if the account was re-authenticated (launch retries). */
    virtual bool reauthenticate(MinecraftInstance* instance, const MinecraftAccountPtr& account, const QString& reason) = 0;

    /** The account cannot play the full game. Return true to launch the demo. */
    virtual bool confirmDemo(MinecraftInstance* instance, bool hasAccount) = 0;

    /** Player name for offline/demo sessions; std::nullopt aborts the launch. */
    virtual std::optional<QString> offlineName(MinecraftInstance* instance, const QString& suggested, const QString& reason) = 0;

    /** The account has no Minecraft profile yet. Return true once one was created. */
    virtual bool setupProfile(MinecraftInstance* instance, const MinecraftAccountPtr& account) = 0;

    /** A non-fatal message the user should see. */
    virtual void showError(MinecraftInstance* instance, const QString& title, const QString& message) = 0;

    /** The instance's console should be brought up ("ShowConsole" / "ShowConsoleOnError"). */
    virtual void showConsole(MinecraftInstance* instance) = 0;

    /** The user asked to stop a running game. Return true to kill it. */
    virtual bool confirmKill(MinecraftInstance* instance) = 0;

    /** A profiler is waiting before the game continues; returns when the launch may proceed. */
    virtual void profilerReady(MinecraftInstance* instance, const QString& message) = 0;

    /** The launch controller finished: succeeded, failed with `reason`, or was aborted. */
    virtual void launchFinished(MinecraftInstance* instance, bool success, bool aborted, const QString& reason) = 0;
};
