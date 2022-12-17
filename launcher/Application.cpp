// SPDX-FileCopyrightText: 2022 Sefa Eyeoglu <contact@scrumplex.net>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 Lenny McLennington <lenny@sneed.church>
 *  Copyright (C) 2022 Tayou <tayou@gmx.net>
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
 *      Copyright 2013-2021 MultiMC Contributors
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

#include "Application.h"
#include "BuildConfig.h"

#include "DataMigrationTask.h"
#include "net/PasteUpload.h"
#include "pathmatcher/MultiMatcher.h"
#include "pathmatcher/SimplePrefixMatcher.h"
#include "ui/MainWindow.h"
#include "ui/InstanceWindow.h"

#include "ui/dialogs/ProgressDialog.h"
#include "ui/instanceview/AccessibleInstanceView.h"

#include "ui/pages/BasePageProvider.h"
#include "ui/pages/global/LauncherPage.h"
#include "ui/pages/global/MinecraftPage.h"
#include "ui/pages/global/JavaPage.h"
#include "ui/pages/global/LanguagePage.h"
#include "ui/pages/global/ProxyPage.h"
#include "ui/pages/global/ExternalToolsPage.h"
#include "ui/pages/global/AccountListPage.h"
#include "ui/pages/global/APIPage.h"
#include "ui/pages/global/CustomCommandsPage.h"

#ifdef Q_OS_WIN
#include "ui/WinDarkmode.h"
#include <versionhelpers.h>
#endif

#include "ui/setupwizard/SetupWizard.h"
#include "ui/setupwizard/LanguageWizardPage.h"
#include "ui/setupwizard/JavaWizardPage.h"
#include "ui/setupwizard/PasteWizardPage.h"

#include "ui/dialogs/CustomMessageBox.h"

#include "ui/pagedialog/PageDialog.h"

#include "ui/themes/ThemeManager.h"

#include "ApplicationMessage.h"

#include <iostream>

#include <QAccessible>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QTranslator>
#include <QLibraryInfo>
#include <QList>
#include <QStringList>
#include <QDebug>
#include <QStyleFactory>
#include <QWindow>
#include <QIcon>

#include "InstanceList.h"
#include "MTPixmapCache.h"

#include <minecraft/auth/AccountList.h>
#include "icons/IconList.h"
#include "net/HttpMetaCache.h"

#include "java/JavaUtils.h"

#include "updater/UpdateChecker.h"

#include "tools/JProfiler.h"
#include "tools/JVisualVM.h"
#include "tools/MCEditTool.h"

#include "settings/INISettingsObject.h"
#include "settings/Setting.h"

#include "translations/TranslationsModel.h"
#include "meta/Index.h"

#include <FileSystem.h>
#include <DesktopServices.h>
#include <LocalPeer.h>

#include <sys.h>

#ifdef Q_OS_LINUX
#include <dlfcn.h>
#include "gamemode_client.h"
#include "MangoHud.h"
#endif


#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static const QLatin1String liveCheckFile("live.check");

PixmapCache* PixmapCache::s_instance = nullptr;

namespace {
void appDebugOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const char *levels = "DWCFIS";
    const QString format("%1 %2 %3\n");

    qint64 msecstotal = APPLICATION->timeSinceStart();
    qint64 seconds = msecstotal / 1000;
    qint64 msecs = msecstotal % 1000;
    QString foo;
    char buf[1025] = {0};
    ::snprintf(buf, 1024, "%5lld.%03lld", seconds, msecs);

    QString out = format.arg(buf).arg(levels[type]).arg(msg);

    APPLICATION->logFile->write(out.toUtf8());
    APPLICATION->logFile->flush();
    QTextStream(stderr) << out.toLocal8Bit();
    fflush(stderr);
}

QString getIdealPlatform(QString currentPlatform) {
    auto info = Sys::getKernelInfo();
    switch(info.kernelType) {
        case Sys::KernelType::Darwin: {
            if(info.kernelMajor >= 17) {
                // macOS 10.13 or newer
                return "osx64-5.15.2";
            }
            else {
                // macOS 10.12 or older
                return "osx64";
            }
        }
        case Sys::KernelType::Windows: {
            // FIXME: 5.15.2 is not stable on Windows, due to a large number of completely unpredictable and hard to reproduce issues
            break;
/*
            if(info.kernelMajor == 6 && info.kernelMinor >= 1) {
                // Windows 7
                return "win32-5.15.2";
            }
            else if (info.kernelMajor > 6) {
                // Above Windows 7
                return "win32-5.15.2";
            }
            else {
                // Below Windows 7
                return "win32";
            }
*/
        }
        case Sys::KernelType::Undetermined:
        case Sys::KernelType::Linux: {
            break;
        }
    }
    return currentPlatform;
}

}

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
#if defined Q_OS_WIN32
    // attach the parent console
    if(AttachConsole(ATTACH_PARENT_PROCESS))
    {
        // if attach succeeds, reopen and sync all the i/o
        if(freopen("CON", "w", stdout))
        {
            std::cout.sync_with_stdio();
        }
        if(freopen("CON", "w", stderr))
        {
            std::cerr.sync_with_stdio();
        }
        if(freopen("CON", "r", stdin))
        {
            std::cin.sync_with_stdio();
        }
        auto out = GetStdHandle (STD_OUTPUT_HANDLE);
        DWORD written;
        const char * endline = "\n";
        WriteConsole(out, endline, strlen(endline), &written, NULL);
        consoleAttached = true;
    }
#endif
    setOrganizationName(BuildConfig.LAUNCHER_NAME);
    setOrganizationDomain(BuildConfig.LAUNCHER_DOMAIN);
    setApplicationName(BuildConfig.LAUNCHER_NAME);
    setApplicationDisplayName(QString("%1 %2").arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.printableVersionString()));
    setApplicationVersion(BuildConfig.printableVersionString() + "\n" + BuildConfig.GIT_COMMIT);
    setDesktopFileName(BuildConfig.LAUNCHER_DESKTOPFILENAME);
    startTime = QDateTime::currentDateTime();

    // Don't quit on hiding the last window
    this->setQuitOnLastWindowClosed(false);

    // Commandline parsing
    QCommandLineParser parser;
    parser.setApplicationDescription(BuildConfig.LAUNCHER_DISPLAYNAME);

    parser.addOptions({
        {{"d", "dir"}, "Use a custom path as application root (use '.' for current directory)", "directory"},
        {{"l", "launch"}, "Launch the specified instance (by instance ID)", "instance"},
        {{"s", "server"}, "Join the specified server on launch (only valid in combination with --launch)", "address"},
        {{"a", "profile"}, "Use the account specified by its profile name (only valid in combination with --launch)", "profile"},
        {"alive", "Write a small '" + liveCheckFile + "' file after the launcher starts"},
        {{"I", "import"}, "Import instance from specified zip (local path or URL)", "file"},
        {"show", "Opens the window for the specified instance (by instance ID)", "show"}
    });
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(arguments());

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch = parser.value("launch");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin = parser.value("server");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profileToUse = parser.value("profile");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_liveCheck = parser.isSet("alive");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_zipToImport = parser.value("import");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToShowWindowOf = parser.value("show");

    // error if --launch is missing with --server or --profile
    if((!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin.isEmpty() || !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profileToUse.isEmpty()) && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch.isEmpty())
    {
        std::cerr << "--server and --profile can only be used in combination with --launch!" << std::endl;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status = Application::Failed;
        return;
    }

    QString origcwdPath = QDir::currentPath();
    QString binPath = applicationDirPath();

    {
        // Root path is used for updates and portable data
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        QDir foo(FS::PathCombine(binPath, "..")); // typically portable-root or /usr
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath = binPath;
#elif defined(Q_OS_MAC)
        QDir foo(FS::PathCombine(binPath, "../.."));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath = foo.absolutePath();
        // on macOS, touch the root to force Finder to reload the .app metadata (and fix any icon change issues)
        FS::updateTimestamp(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath);
#endif
    }

    QString adjustedBy;
    QString dataPath;
    // change folder
    QString dirParam = parser.value("dir");
    if (!dirParam.isEmpty())
    {
        // the dir param. it makes multimc data path point to whatever the user specified
        // on command line
        adjustedBy = "Command line";
        dataPath = dirParam;
    }
    else
    {
        QDir foo(FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), ".."));
        dataPath = foo.absolutePath();
        adjustedBy = "Persistent data path";

#ifndef Q_OS_MACOS
        if (QFile::exists(FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath, "portable.txt"))) {
            dataPath = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath;
            adjustedBy = "Portable data path";
        }
#endif
    }

    if (!FS::ensureFolderPathExists(dataPath))
    {
        showFatalErrorMessage(
            "The launcher data folder could not be created.",
            QString(
                "The launcher data folder could not be created.\n"
                "\n"
                "Make sure you have the right permissions to the launcher data folder and any folder needed to access it.\n"
                "(%1)\n"
                "\n"
                "The launcher cannot continue until you fix this problem."
            ).arg(dataPath)
        );
        return;
    }
    if (!QDir::setCurrent(dataPath))
    {
        showFatalErrorMessage(
            "The launcher data folder could not be opened.",
            QString(
                "The launcher data folder could not be opened.\n"
                "\n"
                "Make sure you have the right permissions to the launcher data folder.\n"
                "(%1)\n"
                "\n"
                "The launcher cannot continue until you fix this problem."
            ).arg(dataPath)
        );
        return;
    }

    /*
     * Establish the mechanism for communication with an already running PolyMC that uses the same data path.
     * If there is one, tell it what the user actually wanted to do and exit.
     * We want to initialize this before logging to avoid messing with the log of a potential already running copy.
     */
    auto appID = ApplicationId::fromPathAndVersion(QDir::currentPath(), BuildConfig.printableVersionString());
    {
        // FIXME: you can run the same binaries with multiple data dirs and they won't clash. This could cause issues for updates.
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_peerInstance = new LocalPeer(this, appID);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_peerInstance, &LocalPeer::messageReceived, this, &Application::messageReceived);
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_peerInstance->isClient()) {
            int timeout = 2000;

            if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch.isEmpty())
            {
                ApplicationMessage activate;
                activate.command = "activate";
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_peerInstance->sendMessage(activate.serialize(), timeout);

                if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_zipToImport.isEmpty())
                {
                    ApplicationMessage import;
                    import.command = "import";
                    import.args.insert("path", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_zipToImport.toString());
                    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_peerInstance->sendMessage(import.serialize(), timeout);
                }
            }
            else
            {
                ApplicationMessage launch;
                launch.command = "launch";
                launch.args["id"] = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch;

                if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin.isEmpty())
                {
                    launch.args["server"] = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin;
                }
                if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profileToUse.isEmpty())
                {
                    launch.args["profile"] = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profileToUse;
                }
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_peerInstance->sendMessage(launch.serialize(), timeout);
            }
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status = Application::Succeeded;
            return;
        }
    }

    // init the logger
    {
        static const QString logBase = BuildConfig.LAUNCHER_NAME + "-%0.log";
        auto moveFile = [](const QString &oldName, const QString &newName)
        {
            QFile::remove(newName);
            QFile::copy(oldName, newName);
            QFile::remove(oldName);
        };

        moveFile(logBase.arg(3), logBase.arg(4));
        moveFile(logBase.arg(2), logBase.arg(3));
        moveFile(logBase.arg(1), logBase.arg(2));
        moveFile(logBase.arg(0), logBase.arg(1));

        logFile = std::unique_ptr<QFile>(new QFile(logBase.arg(0)));
        if(!logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            showFatalErrorMessage(
                "The launcher data folder is not writable!",
                QString(
                    "The launcher couldn't create a log file - the data folder is not writable.\n"
                    "\n"
                    "Make sure you have write permissions to the data folder.\n"
                    "(%1)\n"
                    "\n"
                    "The launcher cannot continue until you fix this problem."
                ).arg(dataPath)
            );
            return;
        }
        qInstallMessageHandler(appDebugOutput);
        qDebug() << "<> Log initialized.";
    }

    {
        bool migrated = false;

        if (!migrated)
            migrated = handleDataMigration(dataPath, FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "../../PolyMC"), "PolyMC", "polymc.cfg");
        if (!migrated)
            migrated = handleDataMigration(dataPath, FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "../../multimc"), "MultiMC", "multimc.cfg");
    }

    {

        qDebug() << BuildConfig.LAUNCHER_DISPLAYNAME << ", (c) 2013-2021 " << BuildConfig.LAUNCHER_COPYRIGHT;
        qDebug() << "Version                    : " << BuildConfig.printableVersionString();
        qDebug() << "Git commit                 : " << BuildConfig.GIT_COMMIT;
        qDebug() << "Git refspec                : " << BuildConfig.GIT_REFSPEC;
        if (adjustedBy.size())
        {
            qDebug() << "Work dir before adjustment : " << origcwdPath;
            qDebug() << "Work dir after adjustment  : " << QDir::currentPath();
            qDebug() << "Adjusted by                : " << adjustedBy;
        }
        else
        {
            qDebug() << "Work dir                   : " << QDir::currentPath();
        }
        qDebug() << "Binary path                : " << binPath;
        qDebug() << "Application root path      : " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath;
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch.isEmpty())
        {
            qDebug() << "ID of instance to launch   : " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch;
        }
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin.isEmpty())
        {
            qDebug() << "Address of server to join  :" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin;
        }
        qDebug() << "<> Paths set.";
    }

    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_liveCheck)
    {
        QFile check(liveCheckFile);
        if(check.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            auto payload = appID.toString().toUtf8();
            if(check.write(payload) == payload.size())
            {
                check.close();
            } else {
                qWarning() << "Could not write into" << liveCheckFile << "!";
                check.remove();  // also closes file!
            }
        } else {
            qWarning() << "Could not open" << liveCheckFile << "for writing!";
        }
    }

    // Initialize application settings
    {
        // Provide a fallback for migration from PolyMC
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings.reset(new INISettingsObject({ BuildConfig.LAUNCHER_CONFIGFILE, "polymc.cfg", "multimc.cfg" }, this));
        // Updates
        // Multiple channels are separated by spaces
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("UpdateChannel", BuildConfig.VERSION_CHANNEL);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("AutoUpdate", true);

        // Theming
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("IconTheme", QString("pe_colored"));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ApplicationTheme", QString("system"));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("BackgroundCat", QString("kitteh"));

        // Remembered state
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("LastUsedGroupForNewInstance", QString());

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("MenuBarInsteadOfToolBar", false);

        QString defaultMonospace;
        int defaultSize = 11;
#ifdef Q_OS_WIN32
        defaultMonospace = "Courier";
        defaultSize = 10;
#elif defined(Q_OS_MAC)
        defaultMonospace = "Menlo";
#else
        defaultMonospace = "Monospace";
#endif

        // resolve the font so the default actually matches
        QFont consoleFont;
        consoleFont.setFamily(defaultMonospace);
        consoleFont.setStyleHint(QFont::Monospace);
        consoleFont.setFixedPitch(true);
        QFontInfo consoleFontInfo(consoleFont);
        QString resolvedDefaultMonospace = consoleFontInfo.family();
        QFont resolvedFont(resolvedDefaultMonospace);
        qDebug() << "Detected default console font:" << resolvedDefaultMonospace
            << ", substitutions:" << resolvedFont.substitutions().join(',');

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ConsoleFont", resolvedDefaultMonospace);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ConsoleFontSize", defaultSize);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ConsoleMaxLines", 100000);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ConsoleOverflowStop", true);

        // Folders
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("InstanceDir", "instances");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"CentralModsDir", "ModsDir"}, "mods");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("IconsDir", "icons");

        // Editors
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("JsonEditor", QString());

        // Language
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("Language", QString());

        // Console
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ShowConsole", false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("AutoCloseConsole", false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ShowConsoleOnError", true);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("LogPrePostOutput", true);

        // Window Size
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"LaunchMaximized", "MCWindowMaximize"}, false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"MinecraftWinWidth", "MCWindowWidth"}, 854);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"MinecraftWinHeight", "MCWindowHeight"}, 480);

        // Proxy Settings
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ProxyType", "None");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"ProxyAddr", "ProxyHostName"}, "127.0.0.1");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ProxyPort", 8080);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"ProxyUser", "ProxyUsername"}, "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"ProxyPass", "ProxyPassword"}, "");

        // Memory
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"MinMemAlloc", "MinMemoryAlloc"}, 512);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"MaxMemAlloc", "MaxMemoryAlloc"}, suitableMaxMem());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("PermGen", 128);

        // Java Settings
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("JavaPath", "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("JavaTimestamp", 0);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("JavaArchitecture", "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("JavaRealArchitecture", "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("JavaVersion", "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("JavaVendor", "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("LastHostname", "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("JvmArgs", "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("IgnoreJavaCompatibility", false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("IgnoreJavaWizard", false);

        // Native library workarounds
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("UseNativeOpenAL", false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("UseNativeGLFW", false);

        // Peformance related options
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("EnableFeralGamemode", false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("EnableMangoHud", false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("UseDiscreteGpu", false);

        // Game time
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ShowGameTime", true);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ShowGlobalGameTime", true);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("RecordGameTime", true);

        // Minecraft launch method
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("MCLaunchMethod", "LauncherPart");

        // Minecraft mods
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ModMetadataDisabled", false);

        // Minecraft offline player name
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("LastOfflinePlayerName", "");

        // Wrapper command for launch
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("WrapperCommand", "");

        // Custom Commands
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"PreLaunchCommand", "PreLaunchCmd"}, "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting({"PostExitCommand", "PostExitCmd"}, "");

        // The cat
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("TheCat", false);

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ToolbarsLocked", false);

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("InstSortMode", "Name");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("SelectedInstance", QString());

        // Window state and geometry
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("MainWindowState", "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("MainWindowGeometry", "");

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ConsoleWindowState", "");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ConsoleWindowGeometry", "");

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("SettingsGeometry", "");

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("PagedGeometry", "");

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("NewInstanceGeometry", "");

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("UpdateDialogGeometry", "");

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("ModDownloadGeometry", "");

        // HACK: This code feels so stupid is there a less stupid way of doing this?
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("PastebinURL", "");
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("PastebinType", PasteUpload::PasteType::Mclogs);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("PastebinCustomAPIBase", "");

            QString pastebinURL = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("PastebinURL").toString();

            bool userHadDefaultPastebin = pastebinURL == "https://0x0.st";
            if (!pastebinURL.isEmpty() && !userHadDefaultPastebin)
            {
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("PastebinType", PasteUpload::PasteType::NullPointer);
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("PastebinCustomAPIBase", pastebinURL);
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("PastebinURL");
            }

            bool ok;
            int pasteType = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("PastebinType").toInt(&ok);
            // If PastebinType is invalid then reset the related settings.
            if (!ok || !(PasteUpload::PasteType::First <= pasteType && pasteType <= PasteUpload::PasteType::Last))
            {
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("PastebinType");
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("PastebinCustomAPIBase");
            }
        }
        // meta URL
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("MetaURLOverride", "");

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("CloseAfterLaunch", false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("QuitAfterGameStop", false);

        // Custom Microsoft Authentication Client ID
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("MSAClientIDOverride", "");

        // Custom Flame API Key
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("CFKeyOverride", "");
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("FlameKeyOverride", "");

            QString flameKey = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("CFKeyOverride").toString();

            if (!flameKey.isEmpty())
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("FlameKeyOverride", flameKey);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("CFKeyOverride");
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->registerSetting("UserAgentOverride", "");

        // Init page provider
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider = std::make_shared<GenericPageProvider>(tr("Settings"));
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider->addPage<LauncherPage>();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider->addPage<MinecraftPage>();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider->addPage<JavaPage>();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider->addPage<LanguagePage>();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider->addPage<CustomCommandsPage>();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider->addPage<ProxyPage>();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider->addPage<ExternalToolsPage>();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider->addPage<AccountListPage>();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider->addPage<APIPage>();
        }

        PixmapCache::setInstance(new PixmapCache(this));

        qDebug() << "<> Settings loaded.";
    }

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installFactory(groupViewAccessibleFactory);
#endif /* !QT_NO_ACCESSIBILITY */

    // initialize network access and proxy setup
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network = new QNetworkAccessManager();
        QString proxyTypeStr = settings()->get("ProxyType").toString();
        QString addr = settings()->get("ProxyAddr").toString();
        int port = settings()->get("ProxyPort").value<qint16>();
        QString user = settings()->get("ProxyUser").toString();
        QString pass = settings()->get("ProxyPass").toString();
        updateProxySettings(proxyTypeStr, addr, port, user, pass);
        qDebug() << "<> Network done.";
    }

    // load translations
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_translations.reset(new TranslationsModel("translations"));
        auto bcp47Name = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("Language").toString();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_translations->selectLanguage(bcp47Name);
        qDebug() << "Your language is" << bcp47Name;
        qDebug() << "<> Translations loaded.";
    }

    // initialize the updater
    if(BuildConfig.UPDATER_ENABLED)
    {
        auto platform = getIdealPlatform(BuildConfig.BUILD_PLATFORM);
        auto channelUrl = BuildConfig.UPDATER_BASE + platform + "/channels.json";
        qDebug() << "Initializing updater with platform: " << platform << " -- " << channelUrl;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateChecker.reset(new UpdateChecker(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network, channelUrl, BuildConfig.VERSION_CHANNEL));
        qDebug() << "<> Updater started.";
    }

    // Instance icons
    {
        auto setting = APPLICATION->settings()->getSetting("IconsDir");
        QStringList instFolders =
        {
            ":/icons/multimc/32x32/instances/",
            ":/icons/multimc/50x50/instances/",
            ":/icons/multimc/128x128/instances/",
            ":/icons/multimc/scalable/instances/"
        };
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_icons.reset(new IconList(instFolders, setting->get().toString()));
        connect(setting.get(), &Setting::SettingChanged,[&](const Setting &, QVariant value)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_icons->directoryChanged(value.toString());
        });
        qDebug() << "<> Instance icons intialized.";
    }

    // Themes
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_themeManager = std::make_unique<ThemeManager>(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow);

    // initialize and load all instances
    {
        auto InstDirSetting = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->getSetting("InstanceDir");
        // instance path: check for problems with '!' in instance path and warn the user in the log
        // and remember that we have to show him a dialog when the gui starts (if it does so)
        QString instDir = InstDirSetting->get().toString();
        qDebug() << "Instance path              : " << instDir;
        if (FS::checkProblemticPathJava(QDir(instDir)))
        {
            qWarning() << "Your instance path contains \'!\' and this is known to cause java problems!";
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instances.reset(new InstanceList(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings, instDir, this));
        connect(InstDirSetting.get(), &Setting::SettingChanged, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instances.get(), &InstanceList::on_InstFolderChanged);
        qDebug() << "Loading Instances...";
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instances->loadList();
        qDebug() << "<> Instances loaded.";
    }

    // and accounts
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_accounts.reset(new AccountList(this));
        qDebug() << "Loading accounts...";
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_accounts->setListFilePath("accounts.json", true);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_accounts->loadList();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_accounts->fillQueue();
        qDebug() << "<> Accounts loaded.";
    }

    // init the http meta cache
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache.reset(new HttpMetaCache("metacache"));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("asset_indexes", QDir("assets/indexes").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("asset_objects", QDir("assets/objects").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("versions", QDir("versions").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("libraries", QDir("libraries").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("minecraftforge", QDir("mods/minecraftforge").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("fmllibs", QDir("mods/minecraftforge/libs").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("liteloader", QDir("mods/liteloader").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("general", QDir("cache").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("ATLauncherPacks", QDir("cache/ATLauncherPacks").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("FTBPacks", QDir("cache/FTBPacks").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("ModpacksCHPacks", QDir("cache/ModpacksCHPacks").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("TechnicPacks", QDir("cache/TechnicPacks").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("FlamePacks", QDir("cache/FlamePacks").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("FlameMods", QDir("cache/FlameMods").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("ModrinthPacks", QDir("cache/ModrinthPacks").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("ModrinthModpacks", QDir("cache/ModrinthModpacks").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("root", QDir::currentPath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("translations", QDir("translations").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("icons", QDir("cache/icons").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->addBase("meta", QDir("meta").absolutePath());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache->Load();
        qDebug() << "<> Cache initialized.";
    }

    // now we have network, download translation updates
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_translations->downloadIndex();

    //FIXME: what to do with these?
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profilers.insert("jprofiler", std::shared_ptr<BaseProfilerFactory>(new JProfilerFactory()));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profilers.insert("jvisualvm", std::shared_ptr<BaseProfilerFactory>(new JVisualVMFactory()));
    for (auto profiler : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profilers.values())
    {
        profiler->registerSettings(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings);
    }

    // Create the MCEdit thing... why is this here?
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mcedit.reset(new MCEditTool(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings));
    }

#ifdef Q_OS_MACOS
    connect(this, &Application::clickedOnDock, [this]() {
        this->showMainWindow();
    });
#endif

    connect(this, &Application::aboutToQuit, [this](){
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instances)
        {
            // save any remaining instance state
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instances->saveNow();
        }
        if(logFile)
        {
            logFile->flush();
            logFile->close();
        }
    });

    {
        setIconTheme(settings()->get("IconTheme").toString());
        qDebug() << "<> Icon theme set.";
        setApplicationTheme(settings()->get("ApplicationTheme").toString(), true);
        qDebug() << "<> Application theme set.";
    }

    updateCapabilities();

    if(createSetupWizard())
    {
        return;
    }

    performMainStartupAction();
}

bool Application::createSetupWizard()
{
    bool javaRequired = [&]()
    {
        bool ignoreJavaWizard = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("IgnoreJavaWizard").toBool();
        if(ignoreJavaWizard) {
            return false;
        }
        QString currentHostName = QHostInfo::localHostName();
        QString oldHostName = settings()->get("LastHostname").toString();
        if (currentHostName != oldHostName)
        {
            settings()->set("LastHostname", currentHostName);
            return true;
        }
        QString currentJavaPath = settings()->get("JavaPath").toString();
        QString actualPath = FS::ResolveExecutable(currentJavaPath);
        if (actualPath.isNull())
        {
            return true;
        }
        return false;
    }();
    bool languageRequired = [&]()
    {
        if (settings()->get("Language").toString().isEmpty())
            return true;
        return false;
    }();
    bool pasteInterventionRequired = settings()->get("PastebinURL") != "";
    bool wizardRequired = javaRequired || languageRequired || pasteInterventionRequired;

    if(wizardRequired)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard = new SetupWizard(nullptr);
        if (languageRequired)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard->addPage(new LanguageWizardPage(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard));
        }

        if (javaRequired)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard->addPage(new JavaWizardPage(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard));
        }

        if (pasteInterventionRequired)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard->addPage(new PasteWizardPage(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard));
        }
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard, &QDialog::finished, this, &Application::setupWizardFinished);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard->show();
        return true;
    }
    return false;
}

bool Application::event(QEvent* event)
{
#ifdef Q_OS_MACOS
    if (event->type() == QEvent::ApplicationStateChange) {
        auto ev = static_cast<QApplicationStateChangeEvent*>(event);

        if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prevAppState == Qt::ApplicationActive && ev->applicationState() == Qt::ApplicationActive) {
            emit clickedOnDock();
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prevAppState = ev->applicationState();
    }
#endif

    if (event->type() == QEvent::FileOpen) {
        auto ev = static_cast<QFileOpenEvent*>(event);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->droppedURLs({ ev->url() });
    }

    return QApplication::event(event);
}

void Application::setupWizardFinished(int status)
{
    qDebug() << "Wizard result =" << status;
    performMainStartupAction();
}

void Application::performMainStartupAction()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status = Application::Initialized;
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch.isEmpty())
    {
        auto inst = instances()->getInstanceById(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch);
        if(inst)
        {
            MinecraftServerTargetPtr serverToJoin = nullptr;
            MinecraftAccountPtr accountToUse = nullptr;

            qDebug() << "<> Instance" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch << "launching";
            if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin.isEmpty())
            {
                // FIXME: validate the server string
                serverToJoin.reset(new MinecraftServerTarget(MinecraftServerTarget::parse(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin)));
                qDebug() << "   Launching with server" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin;
            }

            if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profileToUse.isEmpty())
            {
                accountToUse = accounts()->getAccountByProfileName(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profileToUse);
                if(!accountToUse) {
                    return;
                }
                qDebug() << "   Launching with account" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profileToUse;
            }

            launch(inst, true, false, nullptr, serverToJoin, accountToUse);
            return;
        }
    }
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToShowWindowOf.isEmpty())
    {
        auto inst = instances()->getInstanceById(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToShowWindowOf);
        if(inst)
        {
            qDebug() << "<> Showing window of instance " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToShowWindowOf;
            showInstanceWindow(inst);
            return;
        }
    }
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow)
    {
        // normal main window
        showMainWindow(false);
        qDebug() << "<> Main window shown.";
    }
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_zipToImport.isEmpty())
    {
        qDebug() << "<> Importing instance from zip:" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_zipToImport;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->droppedURLs({ hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_zipToImport });
    }
}

void Application::showFatalErrorMessage(const QString& title, const QString& content)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status = Application::Failed;
    auto dialog = CustomMessageBox::selectable(nullptr, title, content, QMessageBox::Critical);
    dialog->exec();
}

Application::~Application()
{
    // Shut down logger by setting the logger function to nothing
    qInstallMessageHandler(nullptr);

#if defined Q_OS_WIN32
    // Detach from Windows console
    if(consoleAttached)
    {
        fclose(stdout);
        fclose(stdin);
        fclose(stderr);
        FreeConsole();
    }
#endif
}

void Application::messageReceived(const QByteArray& message)
{
    if(status() != Initialized)
    {
        qDebug() << "Received message" << message << "while still initializing. It will be ignored.";
        return;
    }

    ApplicationMessage received;
    received.parse(message);

    auto & command = received.command;

    if(command == "activate")
    {
        showMainWindow();
    }
    else if(command == "import")
    {
        QString path = received.args["path"];
        if(path.isEmpty())
        {
            qWarning() << "Received" << command << "message without a zip path/URL.";
            return;
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->droppedURLs({ QUrl(path) });
    }
    else if(command == "launch")
    {
        QString id = received.args["id"];
        QString server = received.args["server"];
        QString profile = received.args["profile"];

        InstancePtr instance;
        if(!id.isEmpty()) {
            instance = instances()->getInstanceById(id);
            if(!instance) {
                qWarning() << "Launch command requires an valid instance ID. " << id << "resolves to nothing.";
                return;
            }
        }
        else {
            qWarning() << "Launch command called without an instance ID...";
            return;
        }

        MinecraftServerTargetPtr serverObject = nullptr;
        if(!server.isEmpty()) {
            serverObject = std::make_shared<MinecraftServerTarget>(MinecraftServerTarget::parse(server));
        }

        MinecraftAccountPtr accountObject;
        if(!profile.isEmpty()) {
            accountObject = accounts()->getAccountByProfileName(profile);
            if(!accountObject) {
                qWarning() << "Launch command requires the specified profile to be valid. " << profile << "does not resolve to any account.";
                return;
            }
        }

        launch(
            instance,
            true,
            false,
            nullptr,
            serverObject,
            accountObject
        );
    }
    else
    {
        qWarning() << "Received invalid message" << message;
    }
}

std::shared_ptr<TranslationsModel> Application::translations()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_translations;
}

std::shared_ptr<JavaInstallList> Application::javalist()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javalist)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javalist.reset(new JavaInstallList());
    }
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javalist;
}

QList<ITheme*> Application::getValidApplicationThemes()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_themeManager->getValidApplicationThemes();
}

void Application::setApplicationTheme(const QString& name, bool initial)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_themeManager->setApplicationTheme(name, initial);
}

void Application::setIconTheme(const QString& name)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_themeManager->setIconTheme(name);
}

QIcon Application::getThemedIcon(const QString& name)
{
    if(name == "logo") {
        return QIcon(":/" + BuildConfig.LAUNCHER_SVGFILENAME);
    }
    return QIcon::fromTheme(name);
}

bool Application::openJsonEditor(const QString &filename)
{
    const QString file = QDir::current().absoluteFilePath(filename);
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("JsonEditor").toString().isEmpty())
    {
        return DesktopServices::openUrl(QUrl::fromLocalFile(file));
    }
    else
    {
        //return DesktopServices::openFile(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("JsonEditor").toString(), file);
        return DesktopServices::run(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("JsonEditor").toString(), {file});
    }
}

bool Application::launch(
        InstancePtr instance,
        bool online,
        bool demo,
        BaseProfilerFactory *profiler,
        MinecraftServerTargetPtr serverToJoin,
        MinecraftAccountPtr accountToUse
) {
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateRunning)
    {
        qDebug() << "Cannot launch instances while an update is running. Please try again when updates are completed.";
    }
    else if(instance->canLaunch())
    {
        auto & extras = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceExtras[instance->id()];
        auto & window = extras.window;
        if(window)
        {
            if(!window->saveAll())
            {
                return false;
            }
        }
        auto & controller = extras.controller;
        controller.reset(new LaunchController());
        controller->setInstance(instance);
        controller->setOnline(online);
        controller->setDemo(demo);
        controller->setProfiler(profiler);
        controller->setServerToJoin(serverToJoin);
        controller->setAccountToUse(accountToUse);
        if(window)
        {
            controller->setParentWidget(window);
        }
        else if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow)
        {
            controller->setParentWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow);
        }
        connect(controller.get(), &LaunchController::succeeded, this, &Application::controllerSucceeded);
        connect(controller.get(), &LaunchController::failed, this, &Application::controllerFailed);
        connect(controller.get(), &LaunchController::aborted, this, [this] {
            controllerFailed(tr("Aborted"));
        });
        addRunningInstance();
        controller->start();
        return true;
    }
    else if (instance->isRunning())
    {
        showInstanceWindow(instance, "console");
        return true;
    }
    else if (instance->canEdit())
    {
        showInstanceWindow(instance);
        return true;
    }
    return false;
}

bool Application::kill(InstancePtr instance)
{
    if (!instance->isRunning())
    {
        qWarning() << "Attempted to kill instance" << instance->id() << ", which isn't running.";
        return false;
    }
    auto & extras = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceExtras[instance->id()];
    // NOTE: copy of the shared pointer keeps it alive
    auto controller = extras.controller;
    if(controller)
    {
        return controller->abort();
    }
    return true;
}

void Application::closeCurrentWindow()
{
    if (focusWindow())
        focusWindow()->close();
}

void Application::addRunningInstance()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_runningInstances ++;
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_runningInstances == 1)
    {
        emit updateAllowedChanged(false);
    }
}

void Application::subRunningInstance()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_runningInstances == 0)
    {
        qCritical() << "Something went really wrong and we now have less than 0 running instances... WTF";
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_runningInstances --;
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_runningInstances == 0)
    {
        emit updateAllowedChanged(true);
    }
}

bool Application::shouldExitNow() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_runningInstances == 0 && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_openWindows == 0;
}

bool Application::updatesAreAllowed()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_runningInstances == 0;
}

void Application::updateIsRunning(bool running)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateRunning = running;
}


void Application::controllerSucceeded()
{
    auto controller = qobject_cast<LaunchController *>(QObject::sender());
    if(!controller)
        return;
    auto id = controller->id();
    auto & extras = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceExtras[id];

    // on success, do...
    if (controller->instance()->settings()->get("AutoCloseConsole").toBool())
    {
        if(extras.window)
        {
            extras.window->close();
        }
    }
    extras.controller.reset();
    subRunningInstance();

    // quit when there are no more windows.
    if(shouldExitNow())
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status = Status::Succeeded;
        exit(0);
    }
}

void Application::controllerFailed(const QString& error)
{
    Q_UNUSED(error);
    auto controller = qobject_cast<LaunchController *>(QObject::sender());
    if(!controller)
        return;
    auto id = controller->id();
    auto & extras = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceExtras[id];

    // on failure, do... nothing
    extras.controller.reset();
    subRunningInstance();

    // quit when there are no more windows.
    if(shouldExitNow())
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status = Status::Failed;
        exit(1);
    }
}

void Application::ShowGlobalSettings(class QWidget* parent, QString open_page)
{
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider) {
        return;
    }
    emit globalSettingsAboutToOpen();
    {
        SettingsObject::Lock lock(APPLICATION->settings());
        PageDialog dlg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider.get(), open_page, parent);
        dlg.exec();
    }
    emit globalSettingsClosed();
}

MainWindow* Application::showMainWindow(bool minimized)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->setWindowState(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->windowState() & ~Qt::WindowMinimized);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->raise();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->activateWindow();
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow = new MainWindow();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->restoreState(QByteArray::fromBase64(APPLICATION->settings()->get("MainWindowState").toByteArray()));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("MainWindowGeometry").toByteArray()));
#ifdef Q_OS_WIN
        if (IsWindows10OrGreater())
        {
            if (QString::compare(settings()->get("ApplicationTheme").toString(), "dark") == 0) {
                WinDarkmode::setDarkWinTitlebar(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->winId(), true);
            } else {
                WinDarkmode::setDarkWinTitlebar(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->winId(), false);
            }
        }
#endif
        if(minimized)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->showMinimized();
        }
        else
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->show();
        }

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow->checkInstancePathForProblems();
        connect(this, &Application::updateAllowedChanged, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow, &MainWindow::updatesAllowedChanged);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow, &MainWindow::isClosing, this, &Application::on_windowClose);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_openWindows++;
    }
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow;
}

InstanceWindow *Application::showInstanceWindow(InstancePtr instance, QString page)
{
    if(!instance)
        return nullptr;
    auto id = instance->id();
    auto & extras = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceExtras[id];
    auto & window = extras.window;

    if(window)
    {
        window->raise();
        window->activateWindow();
    }
    else
    {
        window = new InstanceWindow(instance);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_openWindows ++;
        connect(window, &InstanceWindow::isClosing, this, &Application::on_windowClose);
    }
    if(!page.isEmpty())
    {
        window->selectPage(page);
    }
    if(extras.controller)
    {
        extras.controller->setParentWidget(window);
    }
    return window;
}

void Application::on_windowClose()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_openWindows--;
    auto instWindow = qobject_cast<InstanceWindow *>(QObject::sender());
    if(instWindow)
    {
        auto & extras = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceExtras[instWindow->instanceId()];
        extras.window = nullptr;
        if(extras.controller)
        {
            extras.controller->setParentWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow);
        }
    }
    auto mainWindow = qobject_cast<MainWindow *>(QObject::sender());
    if(mainWindow)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow = nullptr;
    }
    // quit when there are no more windows.
    if(shouldExitNow())
    {
        exit(0);
    }
}

void Application::updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password)
{
    // Set the application proxy settings.
    if (proxyTypeStr == "SOCKS5")
    {
        QNetworkProxy::setApplicationProxy(
            QNetworkProxy(QNetworkProxy::Socks5Proxy, addr, port, user, password));
    }
    else if (proxyTypeStr == "HTTP")
    {
        QNetworkProxy::setApplicationProxy(
            QNetworkProxy(QNetworkProxy::HttpProxy, addr, port, user, password));
    }
    else if (proxyTypeStr == "None")
    {
        // If we have no proxy set, set no proxy and return.
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    }
    else
    {
        // If we have "Default" selected, set Qt to use the system proxy settings.
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    }

    qDebug() << "Detecting proxy settings...";
    QNetworkProxy proxy = QNetworkProxy::applicationProxy();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network->setProxy(proxy);

    QString proxyDesc;
    if (proxy.type() == QNetworkProxy::NoProxy)
    {
        qDebug() << "Using no proxy is an option!";
        return;
    }
    switch (proxy.type())
    {
    case QNetworkProxy::DefaultProxy:
        proxyDesc = "Default proxy: ";
        break;
    case QNetworkProxy::Socks5Proxy:
        proxyDesc = "Socks5 proxy: ";
        break;
    case QNetworkProxy::HttpProxy:
        proxyDesc = "HTTP proxy: ";
        break;
    case QNetworkProxy::HttpCachingProxy:
        proxyDesc = "HTTP caching: ";
        break;
    case QNetworkProxy::FtpCachingProxy:
        proxyDesc = "FTP caching: ";
        break;
    default:
        proxyDesc = "DERP proxy: ";
        break;
    }
    proxyDesc += QString("%1:%2")
                     .arg(proxy.hostName())
                     .arg(proxy.port());
    qDebug() << proxyDesc;
}

shared_qobject_ptr< HttpMetaCache > Application::metacache()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache;
}

shared_qobject_ptr<QNetworkAccessManager> Application::network()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network;
}

shared_qobject_ptr<Meta::Index> Application::metadataIndex()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metadataIndex)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metadataIndex.reset(new Meta::Index());
    }
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metadataIndex;
}

void Application::updateCapabilities()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capabilities = None;
    if (!getMSAClientID().isEmpty())
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capabilities |= SupportsMSA;
    if (!getFlameAPIKey().isEmpty())
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capabilities |= SupportsFlame;

#ifdef Q_OS_LINUX
    if (gamemode_query_status() >= 0)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capabilities |= SupportsGameMode;

    if (!MangoHud::getLibraryString().isEmpty())
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capabilities |= SupportsMangoHud;
#endif
}

QString Application::getJarPath(QString jarFile)
{
    QStringList potentialPaths = {
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath, "share/" + BuildConfig.LAUNCHER_APP_BINARY_NAME),
#endif
        FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath, "jars"),
        FS::PathCombine(applicationDirPath(), "jars")
    };
    for(QString p : potentialPaths)
    {
        QString jarPath = FS::PathCombine(p, jarFile);
        if (QFileInfo(jarPath).isFile())
            return jarPath;
    }
    return {};
}

QString Application::getMSAClientID()
{
    QString clientIDOverride = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("MSAClientIDOverride").toString();
    if (!clientIDOverride.isEmpty()) {
        return clientIDOverride;
    }

    return BuildConfig.MSA_CLIENT_ID;
}

QString Application::getFlameAPIKey()
{
    QString keyOverride = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("FlameKeyOverride").toString();
    if (!keyOverride.isEmpty()) {
        return keyOverride;
    }

    return BuildConfig.FLAME_API_KEY;
}

QString Application::getUserAgent()
{
    QString uaOverride = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("UserAgentOverride").toString();
    if (!uaOverride.isEmpty()) {
        return uaOverride.replace("$LAUNCHER_VER", BuildConfig.printableVersionString());
    }

    return BuildConfig.USER_AGENT;
}

QString Application::getUserAgentUncached()
{
    QString uaOverride = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("UserAgentOverride").toString();
    if (!uaOverride.isEmpty()) {
        uaOverride += " (Uncached)";
        return uaOverride.replace("$LAUNCHER_VER", BuildConfig.printableVersionString());
    }

    return BuildConfig.USER_AGENT_UNCACHED;
}

int Application::suitableMaxMem()
{
    float totalRAM = (float)Sys::getSystemRam() / (float)Sys::mebibyte;
    int maxMemoryAlloc;

    // If totalRAM < 6GB, use (totalRAM / 1.5), else 4GB
    if (totalRAM < (4096 * 1.5))
        maxMemoryAlloc = (int) (totalRAM / 1.5);
    else
        maxMemoryAlloc = 4096;

    return maxMemoryAlloc;
}

bool Application::handleDataMigration(const QString& currentData,
                                      const QString& oldData,
                                      const QString& name,
                                      const QString& configFile) const
{
    QString nomigratePath = FS::PathCombine(currentData, name + "_nomigrate.txt");
    QStringList configPaths = { FS::PathCombine(oldData, configFile), FS::PathCombine(oldData, BuildConfig.LAUNCHER_CONFIGFILE) };

    QLocale locale;

    // Is there a valid config at the old location?
    bool configExists = false;
    for (QString configPath : configPaths) {
        configExists |= QFileInfo::exists(configPath);
    }

    if (!configExists || QFileInfo::exists(nomigratePath)) {
        qDebug() << "<> No migration needed from" << name;
        return false;
    }

    QString message;
    bool currentExists = QFileInfo::exists(FS::PathCombine(currentData, BuildConfig.LAUNCHER_CONFIGFILE));

    if (currentExists) {
        message = tr("Old data from %1 was found, but you already have existing data for %2. Sadly you will need to migrate yourself. Do "
                     "you want to be reminded of the pending data migration next time you start %2?")
                      .arg(name, BuildConfig.LAUNCHER_DISPLAYNAME);
    } else {
        message = tr("It looks like you used %1 before. Do you want to migrate your data to the new location of %2?")
                      .arg(name, BuildConfig.LAUNCHER_DISPLAYNAME);

        QFileInfo logInfo(FS::PathCombine(oldData, name + "-0.log"));
        if (logInfo.exists()) {
            QString lastModified = logInfo.lastModified().toString(locale.dateFormat());
            message = tr("It looks like you used %1 on %2 before. Do you want to migrate your data to the new location of %3?")
                          .arg(name, lastModified, BuildConfig.LAUNCHER_DISPLAYNAME);
        }
    }

    QMessageBox::StandardButton askMoveDialogue =
        QMessageBox::question(nullptr, BuildConfig.LAUNCHER_DISPLAYNAME, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    auto setDoNotMigrate = [&nomigratePath] {
        QFile file(nomigratePath);
        file.open(QIODevice::WriteOnly);
    };

    // create no-migrate file if user doesn't want to migrate
    if (askMoveDialogue != QMessageBox::Yes) {
        qDebug() << "<> Migration declined for" << name;
        setDoNotMigrate();
        return currentExists;  // cancel further migrations, if we already have a data directory
    }

    if (!currentExists) {
        // Migrate!
        auto matcher = std::make_shared<MultiMatcher>();
        matcher->add(std::make_shared<SimplePrefixMatcher>(configFile));
        matcher->add(std::make_shared<SimplePrefixMatcher>(
            BuildConfig.LAUNCHER_CONFIGFILE));  // it's possible that we already used that directory before
        matcher->add(std::make_shared<SimplePrefixMatcher>("accounts.json"));
        matcher->add(std::make_shared<SimplePrefixMatcher>("accounts/"));
        matcher->add(std::make_shared<SimplePrefixMatcher>("assets/"));
        matcher->add(std::make_shared<SimplePrefixMatcher>("icons/"));
        matcher->add(std::make_shared<SimplePrefixMatcher>("instances/"));
        matcher->add(std::make_shared<SimplePrefixMatcher>("libraries/"));
        matcher->add(std::make_shared<SimplePrefixMatcher>("mods/"));
        matcher->add(std::make_shared<SimplePrefixMatcher>("themes/"));

        ProgressDialog diag;
        DataMigrationTask task(nullptr, oldData, currentData, matcher);
        if (diag.execWithTask(&task)) {
            qDebug() << "<> Migration succeeded";
            setDoNotMigrate();
        } else {
            QString reason = task.failReason();
            QMessageBox::critical(nullptr, BuildConfig.LAUNCHER_DISPLAYNAME, tr("Migration failed! Reason: %1").arg(reason));
        }
    } else {
        qWarning() << "<> Migration was skipped, due to existing data";
    }
    return true;
}
