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

class IconStore;
class JavaEnvironment;
class LaunchCoordinator;
class LauncherSettings;
class LauncherWindows;
class NetworkContext;

namespace Meta {
class Index;
}

/**
 * Native libraries and capability flags detected at startup.
 * Application fills this in; instances read it at launch time, so it must stay alive
 * and be updated in place rather than copied into each instance.
 */
struct LauncherRuntimeInfo {
    QString detectedOpenALPath;
    QString detectedGLFWPath;
    QString detectedSDLPath;
    bool supportsMangoHud = false;
};

/**
 * Dependencies of instance load and launch.
 * Pointers are non-owning and must outlive every MinecraftInstance built from this value.
 * A null pointer means that service is unavailable (headless tests); callers skip the work.
 */
struct InstanceServices {
    LauncherSettings* settings = nullptr;
    NetworkContext* network = nullptr;
    IconStore* icons = nullptr;
    Meta::Index* metadata = nullptr;
    const LauncherRuntimeInfo* runtime = nullptr;
    LaunchCoordinator* launch = nullptr;
    LauncherWindows* windows = nullptr;
    JavaEnvironment* java = nullptr;
};
