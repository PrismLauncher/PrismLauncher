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
#include "DesktopServices.h"

namespace {

QString fromStdPath(const std::filesystem::path& path)
{
    return [&path](const auto& s) -> QString {
        if constexpr (std::is_same_v<decltype(s), std::string>) {
            return QString::fromStdString(s);
        } else if constexpr (std::is_same_v<decltype(s), std::wstring>) {
            return QString::fromStdWString(s);
        } else if constexpr (std::is_same_v<decltype(s), std::u16string>) {
            return QString::fromStdU16String(s);
        } else if constexpr (std::is_same_v<decltype(s), std::u32string>) {
            return QString::fromStdU32String(s);
        } else {
            return QString::fromStdString(path.string());
        }
    }(path.native());
}

std::filesystem::path toStdPath(const QString& path)
{
    if constexpr (std::is_same_v<std::filesystem::path::string_type, std::wstring>) {
        return path.toStdWString();
    } else if constexpr (std::is_same_v<std::filesystem::path::string_type, std::u16string>) {
        return path.toStdU16String();
    } else if constexpr (std::is_same_v<std::filesystem::path::string_type, std::u32string>) {
        return path.toStdU32String();
    } else {
        return path.toStdString();
    }
}

}  // namespace

namespace Startup {

/// only use this to find the app path before setting up a proper QApplication
/// during startup
std::filesystem::path resolveApplicationFilePath(const char* argv0)
{
    // this is a bit of cheating but this is actually *really* complicated
    // I don't feel like manually implementing all the ways to fetch the correct path
    // and then fall back to using argv0 just to avoid instating a QApp here
    // Qt has done the hard work

    // shorten the args passe to the temp QApp
    std::string arg0{ argv0 };
    std::array<char*, 2> qAppArgs = { arg0.data(), nullptr };
    int qAppArgsC = 1;
    QCoreApplication app(qAppArgsC, qAppArgs.data());

    return toStdPath(QCoreApplication::applicationFilePath());
}

DataPathResult resolveDataPath(const std::filesystem::path& rootPath)
{
    // use qt impl
    auto origcwdPath = std::filesystem::current_path();
    DataPathSource source = DataPathSource::CurrentWorkingDir;
    std::filesystem::path dataPath = origcwdPath;
    if (auto dataDirEnv = QProcessEnvironment::systemEnvironment().value(QString("%1_DATA_DIR").arg(BuildConfig.LAUNCHER_NAME.toUpper()));
        !dataDirEnv.isEmpty()) {
        dataPath = toStdPath(dataDirEnv);
        source = DataPathSource::SystemEnvironment;
    } else if (DesktopServices::isSnap()) {
        if (auto snapPath = qEnvironmentVariable("SNAP_USER_COMMON"); !snapPath.isEmpty()) {
            dataPath = toStdPath(snapPath);
            source = DataPathSource::PersistentDataPath;
        }
    } else if (auto standardLoc = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation); !standardLoc.isEmpty()) {
        // make absolute and strip off Qt's auto added org and binary name
        dataPath = std::filesystem::absolute(toStdPath(standardLoc));

        auto endsWithLauncherOrOrg = [](const std::filesystem::path& path) -> bool {
            auto fileName = fromStdPath(path.filename()).toLower();
            return fileName == BuildConfig.LAUNCHER_NAME.toLower() || fileName == QCoreApplication::applicationName().toLower() ||
                   fileName == QCoreApplication::organizationName().toLower();
        };

        while (endsWithLauncherOrOrg(dataPath)) {
            dataPath = dataPath.parent_path();
        }

        dataPath = dataPath / BuildConfig.LAUNCHER_NAME.toStdU16String();
        source = DataPathSource::PersistentDataPath;
    }

#ifndef Q_OS_MACOS
    auto root = QString::fromStdWString(rootPath.wstring());
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
