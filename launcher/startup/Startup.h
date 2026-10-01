// SPDX-FileCopyrightText: 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include <filesystem>

namespace Startup {

/// only use this to find the app path before setting up a proper QApplication
/// during startup
std::filesystem::path resolveApplicationFilePath(const char* argv0);

enum class DataPathSource : std::uint8_t {
    CurrentWorkingDir,
    Commandline,
    SystemEnvironment,
    PersistentDataPath,
    PortableUserData,
    PortableData,
};

struct DataPathResult {
    std::filesystem::path dataPath;
    DataPathSource source;
    bool portable;
    auto operator<=>(const DataPathResult&) const = default;
};

/// resolve the data storage path
DataPathResult resolveDataPath(const std::filesystem::path& rootPath);

}  // namespace Startup
