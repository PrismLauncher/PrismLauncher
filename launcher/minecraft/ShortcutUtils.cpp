// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2025 Yihe Li <winmikedows@hotmail.com>
 *
 *  parent program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  parent program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with parent program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * parent file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use parent file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ShortcutUtils.h"

#include "FileSystem.h"

#include <QApplication>
#include <QFileDialog>

#include <BuildConfig.h>
#include <DesktopServices.h>
#include <icons/IconList.h>

namespace ShortcutUtils {

bool createInstanceShortcut(const Shortcut& shortcut, const QString& filePath)
{
    if (!shortcut.instance)
        return false;

    QString appPath = QApplication::applicationFilePath();
    auto icon = APPLICATION->icons()->icon(shortcut.iconKey.isEmpty() ? shortcut.instance->iconKey() : shortcut.iconKey);
    if (icon == nullptr) {
        icon = APPLICATION->icons()->icon("grass");
    }
    QString iconPath;
    QStringList args;
#if defined(Q_OS_MACOS)
    if (appPath.startsWith("/private/var/")) {
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"),
                              QObject::tr("The launcher is in the folder it was extracted from, therefore it cannot create shortcuts."));
        return false;
    }

    iconPath = FS::PathCombine(shortcut.instance->instanceRoot(), "Icon.icns");

    QFile iconFile(iconPath);
    if (!iconFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for application."));
        return false;
    }

    QIcon iconObj = icon->icon();
    bool success = iconObj.pixmap(1024, 1024).save(iconPath, "ICNS");
    iconFile.close();

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for application."));
        return false;
    }
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    if (appPath.startsWith("/tmp/.mount_")) {
        // AppImage!
        appPath = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));
        if (appPath.isEmpty()) {
            QMessageBox::critical(
                shortcut.parent, QObject::tr("Create Shortcut"),
                QObject::tr("Launcher is running as misconfigured AppImage? ($APPIMAGE environment variable is missing)"));
        } else if (appPath.endsWith("/")) {
            appPath.chop(1);
        }
    }

    iconPath = FS::PathCombine(shortcut.instance->instanceRoot(), "icon.png");

    QFile iconFile(iconPath);
    if (!iconFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for shortcut."));
        return false;
    }
    bool success = icon->icon().pixmap(64, 64).save(&iconFile, "PNG");
    iconFile.close();

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for shortcut."));
        return false;
    }

    if (DesktopServices::isFlatpak()) {
        appPath = "flatpak";
        args.append({ "run", BuildConfig.LAUNCHER_APPID });
    }

#elif defined(Q_OS_WIN)
    iconPath = FS::PathCombine(shortcut.instance->instanceRoot(), "icon.ico");

    // part of fix for weird bug involving the window icon being replaced
    // dunno why it happens, but parent 2-line fix seems to be enough, so w/e
    auto appIcon = APPLICATION->logo();

    QFile iconFile(iconPath);
    if (!iconFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for shortcut."));
        return false;
    }
    bool success = icon->icon().pixmap(64, 64).save(&iconFile, "ICO");
    iconFile.close();

    // restore original window icon
    QGuiApplication::setWindowIcon(appIcon);

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for shortcut."));
        return false;
    }

#else
    QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"), QObject::tr("Not supported on your platform!"));
    return false;
#endif
    args.append({ "--launch", shortcut.instance->id() });
    args.append(shortcut.extraArgs);

    QString shortcutPath = FS::createShortcut(filePath, appPath, args, shortcut.name, iconPath);
    if (shortcutPath.isEmpty()) {
#if not defined(Q_OS_MACOS)
        iconFile.remove();
#endif
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"),
                              QObject::tr("Failed to create %1 shortcut!").arg(shortcut.targetString));
        return false;
    }

    shortcut.instance->registerShortcut({ shortcut.name, shortcutPath, shortcut.target });
    return true;
}

bool createInstanceShortcutOnDesktop(const Shortcut& shortcut)
{
    if (!shortcut.instance)
        return false;

    QString desktopDir = FS::getDesktopDir();
    if (desktopDir.isEmpty()) {
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"), QObject::tr("Couldn't find desktop?!"));
        return false;
    }

    QString shortcutFilePath = FS::PathCombine(desktopDir, FS::RemoveInvalidFilenameChars(shortcut.name));
    if (!createInstanceShortcut(shortcut, shortcutFilePath))
        return false;
    QMessageBox::information(shortcut.parent, QObject::tr("Create Shortcut"),
                             QObject::tr("Created a shortcut to this %1 on your desktop!").arg(shortcut.targetString));
    return true;
}

bool createInstanceShortcutInApplications(const Shortcut& shortcut)
{
    if (!shortcut.instance)
        return false;

    QString applicationsDir = FS::getApplicationsDir();
    if (applicationsDir.isEmpty()) {
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"), QObject::tr("Couldn't find applications folder?!"));
        return false;
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    applicationsDir = FS::PathCombine(applicationsDir, BuildConfig.LAUNCHER_DISPLAYNAME + " Instances");

    QDir applicationsDirQ(applicationsDir);
    if (!applicationsDirQ.mkpath(".")) {
        QMessageBox::critical(shortcut.parent, QObject::tr("Create Shortcut"),
                              QObject::tr("Failed to create instances folder in applications folder!"));
        return false;
    }
#endif

    QString shortcutFilePath = FS::PathCombine(applicationsDir, FS::RemoveInvalidFilenameChars(shortcut.name));
    if (!createInstanceShortcut(shortcut, shortcutFilePath))
        return false;
    QMessageBox::information(shortcut.parent, QObject::tr("Create Shortcut"),
                             QObject::tr("Created a shortcut to this %1 in your applications folder!").arg(shortcut.targetString));
    return true;
}

bool createInstanceShortcutInOther(const Shortcut& shortcut)
{
    if (!shortcut.instance)
        return false;

    QString defaultedDir = FS::getDesktopDir();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    QString extension = ".desktop";
#elif defined(Q_OS_WINDOWS)
    QString extension = ".lnk";
#else
    QString extension = "";
#endif

    QString shortcutFilePath = FS::PathCombine(defaultedDir, FS::RemoveInvalidFilenameChars(shortcut.name) + extension);
    QFileDialog fileDialog;
    // workaround to make sure the portal file dialog opens in the desktop directory
    fileDialog.setDirectoryUrl(defaultedDir);

    shortcutFilePath = fileDialog.getSaveFileName(shortcut.parent, QObject::tr("Create Shortcut"), shortcutFilePath,
                                                  QObject::tr("Desktop Entries") + " (*" + extension + ")");
    if (shortcutFilePath.isEmpty())
        return false;  // file dialog canceled by user

    if (shortcutFilePath.endsWith(extension))
        shortcutFilePath = shortcutFilePath.mid(0, shortcutFilePath.length() - extension.length());
    if (!createInstanceShortcut(shortcut, shortcutFilePath))
        return false;
    QMessageBox::information(shortcut.parent, QObject::tr("Create Shortcut"),
                             QObject::tr("Created a shortcut to this %1!").arg(shortcut.targetString));
    return true;
}

}  // namespace ShortcutUtils
