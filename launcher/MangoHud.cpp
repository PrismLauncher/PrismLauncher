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

#include <QDebug>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QSysInfo>
#include <QtGlobal>

#include "FileSystem.h"
#include "Json.h"
#include "MangoHud.h"

#ifdef __GLIBC__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#define UNDEF_GNU_SOURCE
#endif
#include <dlfcn.h>
#include <linux/limits.h>
#endif

namespace MangoHud {

QString getLibraryString()
{
    /**
     * Guess MangoHud install location by searching for vulkan layers in this order:
     *
     * $VK_LAYER_PATH
     * $XDG_DATA_DIRS (/usr/local/share/:/usr/share/)
     * $XDG_DATA_HOME  (~/.local/share)
     * /etc
     * $XDG_CONFIG_DIRS (/etc/xdg)
     * $XDG_CONFIG_HOME (~/.config)
     *
     * @returns Absolute path of libMangoHud.so if found and empty QString otherwise.
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

    for (const QString& vkLayer : vkLayerList) {
        // prefer to use architecture specific vulkan layers
        QString currentArch = QSysInfo::currentCpuArchitecture();

        if (currentArch == "arm64") {
            currentArch = "aarch64";
        }

        QStringList manifestNames = { QString("MangoHud.%1.json").arg(currentArch), "MangoHud.json" };

        QString filePath{};
        for (const QString& manifestName : manifestNames) {
            QString tryPath = FS::PathCombine(vkLayer, manifestName);
            if (QFile::exists(tryPath)) {
                filePath = tryPath;
                break;
            }
        }

        if (filePath.isEmpty()) {
            continue;
        }
        try {
            auto conf = Json::requireDocument(filePath, vkLayer);
            auto confObject = Json::requireObject(conf, vkLayer);
            auto layer = Json::ensureObject(confObject, "layer");
            QString libraryName = Json::ensureString(layer, "library_path");

            if (libraryName.isEmpty()) {
                continue;
            }
            if (QFileInfo(libraryName).isAbsolute()) {
                return libraryName;
            }

#ifdef __GLIBC__
            // Check whether mangohud is usable on a glibc based system
            QString libraryPath = findLibrary(libraryName);
            if (!libraryPath.isEmpty()) {
                return libraryPath;
            }
#else
            // Without glibc return recorded shared library as-is.
            return libraryName;
#endif
        } catch (const Exception& e) {
        }
    }

    return {};
}

QString findLibrary(QString libName)
{
#ifdef __GLIBC__
    const char* library = libName.toLocal8Bit().constData();

    void* handle = dlopen(library, RTLD_NOW);
    if (!handle) {
        qCritical() << "dlopen() failed:" << dlerror();
        return {};
    }

    char path[PATH_MAX];
    if (dlinfo(handle, RTLD_DI_ORIGIN, path) == -1) {
        qCritical() << "dlinfo() failed:" << dlerror();
        dlclose(handle);
        return {};
    }

    auto fullPath = FS::PathCombine(QString(path), libName);

    dlclose(handle);
    return fullPath;
#else
    qWarning() << "MangoHud::findLibrary is not implemented on this platform";
    return {};
#endif
}
}  // namespace MangoHud

#ifdef UNDEF_GNU_SOURCE
#undef _GNU_SOURCE
#undef UNDEF_GNU_SOURCE
#endif
