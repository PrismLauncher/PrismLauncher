// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLauncher - Minecraft Launcher
 *  Copyright (C) 2022 Jan Drögehoff <sentrycraft123@gmail.com>
 *  Copyright (C) 2023 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <QDir>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QSysInfo>
#include <QtGlobal>

#include "BuildConfig.h"
#include "FileSystem.h"
#include "Json.h"
#include "MangoHud.h"

// FIXME: rename this to something generic
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
        // prefer to use architecture specific vulkan layers
        QString currentArch = QSysInfo::currentCpuArchitecture();

        if (currentArch == "arm64") {
            currentArch = "aarch64";
        }

        QStringList manifestNames = { QString("MangoHud.%1.json").arg(currentArch), "MangoHud.json" };

        QString filePath = "";
        for (QString manifestName : manifestNames) {
            QString tryPath = FS::PathCombine(vkLayer, manifestName);
            if (QFile::exists(tryPath)) {
                filePath = tryPath;
                break;
            }
        }

        if (filePath.isEmpty()) {
            continue;
        }

        auto conf = Json::requireDocument(filePath, vkLayer);
        auto confObject = Json::requireObject(conf, vkLayer);
        auto layer = Json::ensureObject(confObject, "layer");
        return Json::ensureString(layer, "library_path");
    }

    return QString();
}

QString getBwrapBinary()
{
    QStringList binaries{ BuildConfig.LINUX_BWRAP_BINARY, "bwrap" };
    for (const auto& binary : binaries) {
        // return if an absolute path is specified (i.e. distribution override)
        if (binary.startsWith('/') && QFileInfo(binary).isExecutable()) {
            qDebug() << "Found absolute bwrap binary" << binary;
            return binary;
        }

        // lookup otherwise
        QString path = QStandardPaths::findExecutable(binary);
        if (!path.isEmpty()) {
            qDebug() << "Found bwrap binary" << binary << "at" << path;
            return path;
        }
    }
    return {};
}

QString getXDGDbusProxyBinary()
{
    QStringList binaries{ BuildConfig.LINUX_DBUSPROXY_BINARY, "xdg-dbus-proxy" };
    for (const auto& binary : binaries) {
        // return if an absolute path is specified (i.e. distribution override)
        if (binary.startsWith('/') && QFileInfo(binary).isExecutable()) {
            qDebug() << "Found absolute xdg-dbus-proxy binary" << binary;
            return binary;
        }

        // lookup otherwise
        QString path = QStandardPaths::findExecutable(binary);
        if (!path.isEmpty()) {
            qDebug() << "Found xdg-dbus-proxy binary" << binary << "at" << path;
            return path;
        }
    }
    return {};
}
}  // namespace MangoHud
