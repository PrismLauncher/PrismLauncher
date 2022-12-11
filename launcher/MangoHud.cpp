// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLauncher - Minecraft Launcher
 *  Copyright (C) 2022 Jan Drögehoff <sentrycraft123@gmail.com>
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

#include <QStringList>
#include <QDir>
#include <QString>
#include <QtGlobal>

#include "MangoHud.h"
#include "FileSystem.h"
#include "Json.h"

namespace MangoHud {

QString getLibraryString()
{
    /*
     * Check for vulkan layers in this order:
     *
     * $VK_LAYER_PATH
     * $XDG_DATA_DIRS (/usr/local/share/:/usr/share/)
     * $XDG_DATA_HOME  (~/.local/share)
     * /etc
     * $XDG_CONFIG_DIRS (/etc/xdg)
     * $XDG_CONFIG_HOME (~/.config)
     */

    QStringList vkLayerList;
    {
        QString home = QDir::homePath();

        QString vkLayerPath = qEnvironmentVariable("VK_LAYER_PATH");
        if (!vkLayerPath.isEmpty()) {
            vkLayerList << vkLayerPath;
        }

        QStringList xdgDataDirs = qEnvironmentVariable("XDG_DATA_DIRS", "/usr/local/share/:/usr/share/").split(QLatin1String(":"));
        for (QString dir : xdgDataDirs) {
            vkLayerList << FS::PathCombine(dir, "vulkan", "implicit_layer.d");
        }

        QString xdgDataHome = qEnvironmentVariable("XDG_DATA_HOME");
        if (xdgDataHome.isEmpty()) {
            xdgDataHome = FS::PathCombine(home, ".local", "share");
        }
        vkLayerList << FS::PathCombine(xdgDataHome, "vulkan", "implicit_layer.d");

        vkLayerList << "/etc";

        QStringList xdgConfigDirs = qEnvironmentVariable("XDG_CONFIG_DIRS", "/etc/xdg").split(QLatin1String(":"));
        for (QString dir : xdgConfigDirs) {
            vkLayerList << FS::PathCombine(dir, "vulkan", "implicit_layer.d");
        }

        QString xdgConfigHome = qEnvironmentVariable("XDG_CONFIG_HOME");
        if (xdgConfigHome.isEmpty()) {
            xdgConfigHome = FS::PathCombine(home, ".config");
        }
        vkLayerList << FS::PathCombine(xdgConfigHome, "vulkan", "implicit_layer.d");
    }

    for (QString vkLayer : vkLayerList) {
        QString filePath = FS::PathCombine(vkLayer, "MangoHud.json");
        if (!QFile::exists(filePath))
            continue;

        auto conf = Json::requireDocument(filePath, vkLayer);
        auto confObject = Json::requireObject(conf, vkLayer);
        auto layer = Json::ensureObject(confObject, "layer");
        return Json::ensureString(layer, "library_path");
    }

    return QString();
}
}  // namespace MangoHud
