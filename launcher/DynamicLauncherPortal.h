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

#include <QObject>
#include <QString>
#include <QEventLoop>
#include <QVariantMap>

namespace DynamicLauncherPortal {

/// A QObject subclass needed to receive the D-Bus Response signal from the portal.
/// Defined in the header so that MOC can process it
class PortalResponseReceiver : public QObject
{
    Q_OBJECT
public:
    explicit PortalResponseReceiver(QObject* parent = nullptr) : QObject(parent) {}

    QEventLoop* loop = nullptr;
    QString* outToken = nullptr;
    bool* outAccepted = nullptr;

public slots:
    void portalResponse(uint responseCode, QVariantMap results)
    {
        if (outAccepted)
            *outAccepted = (responseCode == 0);
        if (outToken && responseCode == 0)
            *outToken = results.value(QStringLiteral("token")).toString();
        if (loop)
            loop->quit();
    }
};

/// Check if the DynamicLauncher portal is available on the session bus
bool isPortalAvailable();

/// Install a shortcut via the DynamicLauncher portal.
/// This shows a confirmation dialog to the user through the portal.
/// @param name   The display name of the shortcut
/// @param iconPath  Path to a PNG icon file (will be read and sent to the portal)
/// @param desktopEntry The contents of the .desktop file (without Name= and Icon= lines)
/// @return true if the launcher was successfully installed
bool installLauncher(const QString& name, const QString& iconPath, const QString& desktopEntry);

/// Remove a previously installed shortcut via the DynamicLauncher portal.
/// @param desktopFileId The .desktop file id (e.g. "org.prismlauncher.PrismLauncher.MyInstance.desktop")
/// @return true if successfully uninstalled
bool uninstallLauncher(const QString& desktopFileId);

}  // namespace DynamicLauncherPortal
