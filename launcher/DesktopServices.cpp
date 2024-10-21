// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 dada513 <dada513@protonmail.com>
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2022 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
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
#include "CleanEnvironment.h"
#include "DesktopServices.h"
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include "FileSystem.h"

#ifdef Q_OS_LINUX
// directly invokes xdg-open to remove environment variables
bool xdgOpen(const QString& url)
{
    QProcess process;

    process.setProcessEnvironment(cleanEnvironment());

    process.start("xdg-open", { url });
    process.waitForFinished(-1);

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
        return true;

    process.waitForReadyRead(-1);
    qWarning() << "Running xdg-open failed. Output:" << QString::fromUtf8(process.readAllStandardError());
    return false;
}
#endif

namespace DesktopServices {
bool openPath(const QFileInfo& path, bool ensureFolderPathExists)
{
    qDebug() << "Opening path" << path;
    if (ensureFolderPathExists) {
        FS::ensureFolderPathExists(path);
    }
    return openUrl(QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath()));
}

bool openPath(const QString& path, bool ensureFolderPathExists)
{
    return openPath(QFileInfo(path), ensureFolderPathExists);
}

bool run(const QString& application, const QStringList& args, const QString& workingDirectory, qint64* pid)
{
    qDebug() << "Running" << application << "with args" << args.join(' ');

    QProcess process;

    process.setProcessEnvironment(cleanEnvironment());

    process.setProgram(application);
    process.setArguments(args);
    process.setWorkingDirectory(workingDirectory);

    return process.startDetached(pid);
}

bool openUrl(const QUrl& url)
{
    qDebug() << "Opening URL" << url.toString();
#ifdef Q_OS_LINUX
    return xdgOpen(url.toString());
#else
    return QDesktopServices::openUrl(url);
#endif
}

bool isFlatpak()
{
#ifdef Q_OS_LINUX
    return QFile::exists("/.flatpak-info");
#else
    return false;
#endif
}

bool isSnap()
{
#ifdef Q_OS_LINUX
    return getenv("SNAP");
#else
    return false;
#endif
}

}  // namespace DesktopServices
