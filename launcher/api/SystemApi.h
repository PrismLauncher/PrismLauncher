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

class MinecraftInstance;

namespace api {

class ApiRouter;
class TaskTracker;

struct HostInfo {
    QString webViewEngine;
    bool devMode = false;
};

/** `system.*` (app info, open folder/URL, clipboard, save dialog, icons) and `tasks.*`. */
void registerSystemApi(ApiRouter* router, TaskTracker* tasks, const HostInfo& host);

/**
 * Resolves a folder target name (see `FolderTarget` in frontend/src/types/system.ts) to a directory.
 * The UI never sends paths; only these names. Throws ApiError for unknown targets / instances.
 */
QString resolveFolderTarget(const QString& target, MinecraftInstance* instance);

}  // namespace api
