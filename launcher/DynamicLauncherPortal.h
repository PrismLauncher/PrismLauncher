// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 utophii <pos18411@gmail.com>
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

#include <QByteArray>
#include <QString>

#include "Result.h"

namespace DynamicLauncherPortal {

/// Check if the DynamicLauncher portal is available on the session bus
bool isPortalAvailable();

/// Build the desktop file id the portal uses for @p name:
/// the launcher app id, a sanitized name and a .desktop suffix
QString buildDesktopFileId(const QString& name);

/// Install a shortcut via the DynamicLauncher portal.
/// This shows a confirmation dialog to the user through the portal.
/// @param name         The display name of the shortcut
/// @param icon         The PNG icon data to send to the portal
/// @param desktopEntry The contents of the .desktop file (without Name= and Icon= lines)
Result<> installLauncher(const QString& name, const QByteArray& icon, const QString& desktopEntry);

}  // namespace DynamicLauncherPortal