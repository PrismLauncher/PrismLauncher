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

#ifdef Q_OS_WIN
#include <windows.h>

#include <propkey.h>
#include <propvarutil.h>

namespace {
struct PropVariantDeleter {
    void operator()(PROPVARIANT* property) const noexcept
    {
        PropVariantClear(property);
        delete property;
    }
};
}  // namespace
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

#ifdef Q_OS_WIN
Result<winrt::com_ptr<IShellLink>> createShellLink(const QString& path,
                                                   const QStringList& arguments,
                                                   const QString& workDir,
                                                   const QString& iconPath)
{
    const auto shellLink = winrt::create_instance<IShellLink>(CLSID_ShellLink);
    const auto escapedArguments = FS::quoteShortcutArgs(arguments);

    try {
        winrt::check_hresult(shellLink->SetPath(path.toStdWString().c_str()));
        winrt::check_hresult(shellLink->SetArguments(escapedArguments.toStdWString().c_str()));
        winrt::check_hresult(shellLink->SetWorkingDirectory(workDir.toStdWString().c_str()));
        winrt::check_hresult(shellLink->SetIconLocation(iconPath.toStdWString().c_str(), 0));
    } catch (const winrt::hresult_error& e) {
        const auto message = e.message();
        return std::unexpected("Error creating shell link: " + message);
    }

    return shellLink;
}

// NOTE(@getchoo): ICustomDestinationList is a peak Win32 caveman API
// There's no real way to edit entries in place, even though it's a glorified list
// So, we just recreate and assign the list every time, because we don't need more as of now
// Future callers be warned(?)
Result<> setJumplist(const QList<JumplistEntry>& entries)
{
    try {
        const auto jumplist = winrt::create_instance<ICustomDestinationList>(CLSID_DestinationList);

        // NOTE(@getchoo): We can ignore the "removed" jump list entires, as they don't apply to "user tasks"
        // https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-icustomdestinationlist-addusertasks
        // If we ever support more than only user tasks, do take this into account
        UINT minSlots;
        winrt::com_ptr<IObjectArray> removed;
        qDebug() << "Beginning jumplist";
        winrt::check_hresult(jumplist->BeginList(&minSlots, IID_PPV_ARGS(removed.put())));

        const auto collection = winrt::create_instance<IObjectCollection>(CLSID_EnumerableObjectCollection);
        const auto path = QCoreApplication::applicationFilePath();
        const auto workDir = QCoreApplication::applicationDirPath();
        for (const auto entry : entries) {
            const auto task = createShellLink(path, entry.arguments, workDir, entry.iconPath);
            if (!task)
                return std::unexpected(task.error());

            const auto properties = task.value().as<IPropertyStore>();

            const auto title = std::unique_ptr<PROPVARIANT, PropVariantDeleter>(new PROPVARIANT{});
            winrt::check_hresult(InitPropVariantFromString(entry.name.toStdWString().c_str(), title.get()));
            winrt::check_hresult(properties->SetValue(PKEY_Title, *title));

            winrt::check_hresult(properties->Commit());

            qDebug() << "Adding" << entry.name << "to task collection";
            winrt::check_hresult(collection->AddObject(task.value().get()));
        }

        if (!entries.isEmpty()) {
            const auto tasks = collection.as<IObjectArray>();
            if (FAILED(jumplist->AddUserTasks(tasks.get()))) {
                return std::unexpected("Tasks for jumplist are invalid!");
            }
        }
        qDebug() << "Committing jumplist.";
        winrt::check_hresult(jumplist->CommitList());
    } catch (const winrt::hresult_error& e) {
        const auto message = e.message();
        return std::unexpected("Unable to set jumplist COM object: " + message);
    }

    qDebug() << "Finished setting jumplist COM object.";
    return {};
}
#endif
}  // namespace DesktopServices
