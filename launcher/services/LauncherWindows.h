// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
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

class MinecraftInstance;
class QWidget;

/**
 * Window navigation the rest of the launcher needs.
 * Application owns the real windows and implements this. Callers do not include UI headers.
 */
class LauncherWindows {
   public:
    virtual ~LauncherWindows() = default;

    virtual void showMainWindow() = 0;
    virtual void showInstanceWindow(MinecraftInstance* instance, const QString& page = QString()) = 0;
    virtual void showLogWindow() = 0;
    virtual void showGlobalSettings(QWidget* parent, const QString& page = QString()) = 0;
};
