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
#include "DesktopServices.h"
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include "FileSystem.h"

/**
 * This shouldn't exist, but until QTBUG-9328 and other unreported bugs are fixed, it needs to be a thing.
 */
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

template <typename T>
bool IndirectOpen(T callable, qint64* pid_forked = nullptr)
{
    auto pid = fork();
    if (pid_forked) {
        if (pid > 0)
            *pid_forked = pid;
        else
            *pid_forked = 0;
    }
    if (pid == -1) {
        qWarning() << "IndirectOpen failed to fork: " << errno;
        return false;
    }
    // child - do the stuff
    if (pid == 0) {
        // unset all this garbage so it doesn't get passed to the child process
        qunsetenv("LD_PRELOAD");
        qunsetenv("LD_LIBRARY_PATH");
        qunsetenv("LD_DEBUG");
        qunsetenv("QT_PLUGIN_PATH");
        qunsetenv("QT_FONTPATH");

        // open the URL
        auto status = callable();

        // detach from the parent process group.
        setsid();

        // die. now. do not clean up anything, it would just hang forever.
        _exit(status ? 0 : 1);
    } else {
        // parent - assume it worked.
        int status;
        while (waitpid(pid, &status, 0)) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status) == 0;
            }
            if (WIFSIGNALED(status)) {
                return false;
            }
        }
        return true;
    }
}
#endif

namespace DesktopServices {
bool openPath(const QFileInfo& path, bool ensureExists)
{
    qDebug() << "Opening path" << path;
    if (ensureExists) {
        FS::ensureFolderPathExists(path);
    }
    return openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}

bool openPath(const QString& path, bool ensureExists)
{
    return openPath(QFileInfo(path), ensureExists);
}

bool run(const QString& application, const QStringList& args, const QString& workingDirectory, qint64* pid)
{
    qDebug() << "Running" << application << "with args" << args.join(' ');
    return QProcess::startDetached(application, args, workingDirectory, pid);
}

bool openUrl(const QUrl& url)
{
    qDebug() << "Opening URL" << url.toString();
    return QDesktopServices::openUrl(url);
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

bool isSandbox()
{
    return isSnap() || isFlatpak();
}

}  // namespace DesktopServices
