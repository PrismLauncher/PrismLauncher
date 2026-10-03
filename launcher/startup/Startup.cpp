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

#include "startup/Startup.h"
#include <array>
#include <filesystem>

#include <QCoreApplication>
#include <QString>
#include <QtSystemDetection>

#include <QDir>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QtEnvironmentVariables>

#include "BuildConfig.h"
#include "FileSystem.h"
#include "DesktopServices.h"

namespace Startup {

/// only use this to find the app path before setting up a proper QApplication
/// during startup
std::filesystem::path resolveApplicationFilePath(const char* argv0)
{
    // this is a bit of cheating but this is actually *reallly* complicated
    // I don't feel like manually implementing all the ways to fetch the correct path
    // and then fall back to using argv0 just to avoid instating a QApp here
    // Qt has done the hard work

    // shorten the args passe to the temp QApp
    std::string arg0{ argv0 };
    std::array<char*, 2> qAppArgs = { arg0.data(), nullptr };
    int qAppArgsC = 1;
    QCoreApplication app(qAppArgsC, qAppArgs.data());

    auto path = QCoreApplication::applicationFilePath().toStdU16String();
    return { path };
}

DataPathResult resolveDataPath(const std::filesystem::path& rootPath)
{
    // use qt impl
    // qt uses utf16 behind the scenes and converting between std::u16string and QString is a reinterpret_cast of the data
    auto origcwdPath = std::filesystem::current_path();
    DataPathSource source = DataPathSource::CurrentWorkingDir;
    std::filesystem::path dataPath = origcwdPath;
    if (auto dataDirEnv =
                   QProcessEnvironment::systemEnvironment().value(QString("%1_DATA_DIR").arg(BuildConfig.LAUNCHER_NAME.toUpper()));
               !dataDirEnv.isEmpty()) {
        dataPath = dataDirEnv.toStdU16String();
        source = DataPathSource::SystemEnvironment;
    } else if (DesktopServices::isSnap()) {
        if (auto snapPath = qEnvironmentVariable("SNAP_USER_COMMON"); !snapPath.isEmpty()) {
            dataPath = snapPath.toStdU16String();
            source = DataPathSource::PersistentDataPath;
        }
    } else if (auto standardLoc = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation); !standardLoc.isEmpty()) {
        // make absolute and strip off Qt's auto added org and binary name
        dataPath = std::filesystem::absolute(std::filesystem::path{standardLoc.toStdU16String()}).parent_path();
        while (dataPath.filename() == BuildConfig.LAUNCHER_NAME.toStdU16String()) {
            dataPath = dataPath.parent_path();
        }
        dataPath = dataPath / BuildConfig.LAUNCHER_NAME.toStdU16String();
        source = DataPathSource::PersistentDataPath;
    }

#ifndef Q_OS_MACOS
    auto root = QString::fromStdU16String(rootPath.u16string());
    if (auto portableUserData = FS::PathCombine(root, "UserData"); QDir(portableUserData).exists()) {
        dataPath = portableUserData.toStdU16String();
        source = DataPathSource::PortableUserData;
    } else if (QFile::exists(FS::PathCombine(root, "portable.txt"))) {
        dataPath = root.toStdU16String();
        source = DataPathSource::PortableData;
    }
#endif

    return {
        .dataPath = dataPath,
        .source = source,
    };
}

}  // namespace Startup
