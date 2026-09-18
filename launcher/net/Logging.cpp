// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#include "net/Logging.h"

Q_LOGGING_CATEGORY(taskNetLogC, "launcher.task.net")
Q_LOGGING_CATEGORY(taskNetGetLogC, "launcher.task.net.get")
Q_LOGGING_CATEGORY(taskNetPostLogC, "launcher.task.net.post")
Q_LOGGING_CATEGORY(taskNetPutLogC, "launcher.task.net.put")
Q_LOGGING_CATEGORY(taskNetPatchLogC, "launcher.task.net.patch")
Q_LOGGING_CATEGORY(taskNetDeleteLogC, "launcher.task.net.delete")
Q_LOGGING_CATEGORY(taskNetHeadLogC, "launcher.task.net.head")
Q_LOGGING_CATEGORY(taskNetOptionsLogC, "launcher.task.net.options")
Q_LOGGING_CATEGORY(taskNetConnectLogC, "launcher.task.net.connect")
Q_LOGGING_CATEGORY(taskNetTraceLogC, "launcher.task.net.trace")
Q_LOGGING_CATEGORY(taskMCSkinsLogC, "launcher.task.minecraft.skins")
Q_LOGGING_CATEGORY(taskMetaCacheLogC, "launcher.task.net.metacache")
Q_LOGGING_CATEGORY(taskHttpMetaCacheLogC, "launcher.task.net.metacache.http")
