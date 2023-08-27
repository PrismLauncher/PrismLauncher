// SPDX-FileCopyrightText: 2022 Sefa Eyeoglu <contact@scrumplex.net>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 Lenny McLennington <lenny@sneed.church>
 *  Copyright (C) 2022 Tayou <git@tayou.org>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
#include "settings/INIFile.h"
#include "ui/InstanceWindow.h"
#include "ui/MainWindow.h"

#include "ui/dialogs/ProgressDialog.h"
#include "ui/instanceview/AccessibleInstanceView.h"

#include "ui/pages/BasePageProvider.h"
#include "ui/pages/global/APIPage.h"
#include "ui/pages/global/AccountListPage.h"
#include "ui/pages/global/CustomCommandsPage.h"
#include "ui/pages/global/ExternalToolsPage.h"
#include "ui/pages/global/JavaPage.h"
#include "ui/pages/global/LanguagePage.h"
#include "ui/pages/global/LauncherPage.h"
#include "ui/pages/global/MinecraftPage.h"
#include "ui/pages/global/ProxyPage.h"

#include "ui/setupwizard/JavaWizardPage.h"
#include "ui/setupwizard/LanguageWizardPage.h"
#include "ui/setupwizard/PasteWizardPage.h"
#include "ui/setupwizard/SetupWizard.h"
#include "ui/setupwizard/ThemeWizardPage.h"

#include "ui/dialogs/CustomMessageBox.h"

#include "ui/pagedialog/PageDialog.h"

#include "ui/themes/ThemeManager.h"

#include "ApplicationMessage.h"

#include <iostream>
#include <mutex>

#include <QAccessible>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QIcon>
#include <QLibraryInfo>
#include <QList>
#include <QNetworkAccessManager>
#include <QStringList>
#include <QStyleFactory>
#include <QTranslator>
#include <QWindow>

#include "InstanceList.h"
#include "MTPixmapCache.h"

#include <minecraft/auth/AccountList.h>
#include "icons/IconList.h"
#include "net/HttpMetaCache.h"

#include "java/JavaUtils.h"

#include "updater/ExternalUpdater.h"

#include "tools/JProfiler.h"
#include "tools/JVisualVM.h"
#include "tools/MCEditTool.h"

#include "settings/INISettingsObject.h"
#include "settings/Setting.h"

#include "meta/Index.h"
#include "translations/TranslationsModel.h"

#include <DesktopServices.h>
#include <FileSystem.h>
#include <LocalPeer.h>

#include <sys.h>

#ifdef Q_OS_LINUX
#include <dlfcn.h>
#include "MangoHud.h"
#include "gamemode_client.h"
#endif

#if defined(Q_OS_MAC) && defined(SPARKLE_ENABLED)
#include "updater/MacSparkleUpdater.h"
#endif

#if defined Q_OS_WIN32
#include "WindowsConsole.h"
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static const QLatin1String liveCheckFile("live.check");

PixmapCache* PixmapCache::s_instance = nullptr;

namespace {

/** This is used so that we can output to the log file in addition to the CLI. */
void appDebugOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static std::mutex loggerMutex;
    const std::lock_guard<std::mutex> lock(loggerMutex);  // synchronized, QFile logFile is not thread-safe

    QString out = qFormatLogMessage(type, context, msg);
    out += QChar::LineFeed;

    APPLICATION->logFile->write(out.toUtf8());
    APPLICATION->logFile->flush();
    QTextStream(stderr) << out.toLocal8Bit();
    fflush(stderr);
}

}  // namespace

Application::Application(int& argc, char** argv) : QApplication(argc, argv)
{
#if defined Q_OS_WIN32
    // attach the parent console if stdout not already captured
    if (AttachWindowsConsole()) {
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

    parser.addOptions(
        { { { "d", "dir" }, "Use a custom path as application root (use '.' for current directory)", "directory" },
          { { "l", "launch" }, "Launch the specified instance (by instance ID)", "instance" },
          { { "s", "server" }, "Join the specified server on launch (only valid in combination with --launch)", "address" },
          { { "a", "profile" }, "Use the account specified by its profile name (only valid in combination with --launch)", "profile" },
          { "alive", "Write a small '" + liveCheckFile + "' file after the launcher starts" },
          { { "I", "import" }, "Import instance or resource from specified local path or URL", "url" },
          { "show", "Opens the window for the specified instance (by instance ID)", "show" } });
    // Has to be positional for some OS to handle that properly
    parser.addPositionalArgument("URL", "Import the resource(s) at the given URL(s) (same as -I / --import)", "[URL...]");

    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(arguments());

    m_instanceIdToLaunch = parser.value("launch");
    m_serverToJoin = parser.value("server");
    m_profileToUse = parser.value("profile");
    m_liveCheck = parser.isSet("alive");

    m_instanceIdToShowWindowOf = parser.value("show");

    for (auto url : parser.values("import")) {
        m_urlsToImport.append(normalizeImportUrl(url));
    }

    // treat unspecified positional arguments as import urls
    for (auto url : parser.positionalArguments()) {
        m_urlsToImport.append(normalizeImportUrl(url));
    }

    // error if --launch is missing with --server or --profile
    if ((!m_serverToJoin.isEmpty() || !m_profileToUse.isEmpty()) && m_instanceIdToLaunch.isEmpty()) {
        std::cerr << "--server and --profile can only be used in combination with --launch!" << std::endl;
        m_status = Application::Failed;
        return;
    }

    QString origcwdPath = QDir::currentPath();
    QString binPath = applicationDirPath();

    {
        // Root path is used for updates and portable data
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        QDir foo(FS::PathCombine(binPath, ".."));  // typically portable-root or /usr
        m_rootPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
        m_rootPath = binPath;
#elif defined(Q_OS_MAC)
        QDir foo(FS::PathCombine(binPath, "../.."));
        m_rootPath = foo.absolutePath();
        // on macOS, touch the root to force Finder to reload the .app metadata (and fix any icon change issues)
        FS::updateTimestamp(m_rootPath);
#endif
    }

    QString adjustedBy;
    QString dataPath;
    // change folder
    QString dirParam = parser.value("dir");
    if (!dirParam.isEmpty()) {
        // the dir param. it makes multimc data path point to whatever the user specified
        // on command line
        adjustedBy = "Command line";
        dataPath = dirParam;
    } else {
        QDir foo;
        if (DesktopServices::isSnap()) {
            foo = QDir(getenv("SNAP_USER_COMMON"));
        } else {
            foo = QDir(FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), ".."));
        }

        dataPath = foo.absolutePath();
        adjustedBy = "Persistent data path";

#ifndef Q_OS_MACOS
        if (QFile::exists(FS::PathCombine(m_rootPath, "portable.txt"))) {
            dataPath = m_rootPath;
            adjustedBy = "Portable data path";
            m_portable = true;
        }
#endif
    }

    if (!FS::ensureFolderPathExists(dataPath)) {
        showFatalErrorMessage(
            "The launcher data folder could not be created.",
            QString("The launcher data folder could not be created.\n"
                    "\n"
                    "Make sure you have the right permissions to the launcher data folder and any folder needed to access it.\n"
                    "(%1)\n"
                    "\n"
                    "The launcher cannot continue until you fix this problem.")
                .arg(dataPath));
        return;
    }
    if (!QDir::setCurrent(dataPath)) {
        showFatalErrorMessage("The launcher data folder could not be opened.",
                              QString("The launcher data folder could not be opened.\n"
                                      "\n"
                                      "Make sure you have the right permissions to the launcher data folder.\n"
                                      "(%1)\n"
                                      "\n"
                                      "The launcher cannot continue until you fix this problem.")
                                  .arg(dataPath));
        return;
    }

    /*
     * Establish the mechanism for communication with an already running PrismLauncher that uses the same data path.
     * If there is one, tell it what the user actually wanted to do and exit.
     * We want to initialize this before logging to avoid messing with the log of a potential already running copy.
     */
    auto appID = ApplicationId::fromPathAndVersion(QDir::currentPath(), BuildConfig.printableVersionString());
    {
        // FIXME: you can run the same binaries with multiple data dirs and they won't clash. This could cause issues for updates.
        m_peerInstance = new LocalPeer(this, appID);
        connect(m_peerInstance, &LocalPeer::messageReceived, this, &Application::messageReceived);
        if (m_peerInstance->isClient()) {
            int timeout = 2000;

            if (m_instanceIdToLaunch.isEmpty()) {
                ApplicationMessage activate;
                activate.command = "activate";
                m_peerInstance->sendMessage(activate.serialize(), timeout);

                if (!m_urlsToImport.isEmpty()) {
                    for (auto url : m_urlsToImport) {
                        ApplicationMessage import;
                        import.command = "import";
                        import.args.insert("url", url.toString());
                        m_peerInstance->sendMessage(import.serialize(), timeout);
                    }
                }
            } else {
                ApplicationMessage launch;
                launch.command = "launch";
                launch.args["id"] = m_instanceIdToLaunch;

                if (!m_serverToJoin.isEmpty()) {
                    launch.args["server"] = m_serverToJoin;
                }
                if (!m_profileToUse.isEmpty()) {
                    launch.args["profile"] = m_profileToUse;
                }
                m_peerInstance->sendMessage(launch.serialize(), timeout);
            }
            m_status = Application::Succeeded;
            return;
        }
    }

    // init the logger
    {
        static const QString baseLogFile = BuildConfig.LAUNCHER_NAME + "-%0.log";
        static const QString logBase = FS::PathCombine("logs", baseLogFile);
        auto moveFile = [](const QString& oldName, const QString& newName) {
            QFile::remove(newName);
            QFile::copy(oldName, newName);
            QFile::remove(oldName);
        };
        if (FS::ensureFolderPathExists("logs")) {  // if this did not fail
            for (auto i = 0; i <= 4; i++)
                if (auto oldName = baseLogFile.arg(i);
                    QFile::exists(oldName))  // do not pointlessly delete new files if the old ones are not there
                    moveFile(oldName, logBase.arg(i));
        }

        for (auto i = 4; i > 0; i--)
            moveFile(logBase.arg(i - 1), logBase.arg(i));

        logFile = std::unique_ptr<QFile>(new QFile(logBase.arg(0)));
        if (!logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            showFatalErrorMessage("The launcher data folder is not writable!",
                                  QString("The launcher couldn't create a log file - the data folder is not writable.\n"
                                          "\n"
                                          "Make sure you have write permissions to the data folder.\n"
                                          "(%1)\n"
                                          "\n"
                                          "The launcher cannot continue until you fix this problem.")
                                      .arg(dataPath));
            return;
        }
        qInstallMessageHandler(appDebugOutput);

        qSetMessagePattern(
            "%{time process}"
            " "
            "%{if-debug}D%{endif}"
            "%{if-info}I%{endif}"
            "%{if-warning}W%{endif}"
            "%{if-critical}C%{endif}"
            "%{if-fatal}F%{endif}"
            " "
            "|"
            " "
            "%{if-category}[%{category}]: %{endif}"
            "%{message}");

        bool foundLoggingRules = false;

        auto logRulesFile = QStringLiteral("qtlogging.ini");
        auto logRulesPath = FS::PathCombine(dataPath, logRulesFile);

        qDebug() << "Testing" << logRulesPath << "...";
        foundLoggingRules = QFile::exists(logRulesPath);

        // search the dataPath()
        // seach app data standard path
        if (!foundLoggingRules && !isPortable() && dirParam.isEmpty()) {
            logRulesPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, FS::PathCombine("..", logRulesFile));
            if (!logRulesPath.isEmpty()) {
                qDebug() << "Found" << logRulesPath << "...";
                foundLoggingRules = true;
            }
        }
        // seach root path
        if (!foundLoggingRules) {
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
            logRulesPath = FS::PathCombine(m_rootPath, "share", BuildConfig.LAUNCHER_NAME, logRulesFile);
#else
            logRulesPath = FS::PathCombine(m_rootPath, logRulesFile);
#endif
            qDebug() << "Testing" << logRulesPath << "...";
            foundLoggingRules = QFile::exists(logRulesPath);
        }

        if (foundLoggingRules) {
            // load and set logging rules
            qDebug() << "Loading logging rules from:" << logRulesPath;
            QSettings loggingRules(logRulesPath, QSettings::IniFormat);
            loggingRules.beginGroup("Rules");
            QStringList rule_names = loggingRules.childKeys();
            QStringList rules;
            qDebug() << "Setting log rules:";
            for (auto rule_name : rule_names) {
                auto rule = QString("%1=%2").arg(rule_name).arg(loggingRules.value(rule_name).toString());
                rules.append(rule);
                qDebug() << "    " << rule;
            }
            auto rules_str = rules.join("\n");
            QLoggingCategory::setFilterRules(rules_str);
        }

        qDebug() << "<> Log initialized.";
    }

    {
        bool migrated = false;

        if (!migrated)
            migrated = handleDataMigration(
                dataPath, FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "../../PolyMC"), "PolyMC",
                "polymc.cfg");
        if (!migrated)
            migrated = handleDataMigration(
                dataPath, FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "../../multimc"), "MultiMC",
                "multimc.cfg");
    }

    {
        qDebug() << BuildConfig.LAUNCHER_DISPLAYNAME << ", (c) 2013-2021 " << BuildConfig.LAUNCHER_COPYRIGHT;
        qDebug() << "Version                    : " << BuildConfig.printableVersionString();
        qDebug() << "Platform                   : " << BuildConfig.BUILD_PLATFORM;
        qDebug() << "Git commit                 : " << BuildConfig.GIT_COMMIT;
        qDebug() << "Git refspec                : " << BuildConfig.GIT_REFSPEC;
        if (adjustedBy.size()) {
            qDebug() << "Work dir before adjustment : " << origcwdPath;
            qDebug() << "Work dir after adjustment  : " << QDir::currentPath();
            qDebug() << "Adjusted by                : " << adjustedBy;
        } else {
            qDebug() << "Work dir                   : " << QDir::currentPath();
        }
        qDebug() << "Binary path                : " << binPath;
        qDebug() << "Application root path      : " << m_rootPath;
        if (!m_instanceIdToLaunch.isEmpty()) {
            qDebug() << "ID of instance to launch   : " << m_instanceIdToLaunch;
        }
        if (!m_serverToJoin.isEmpty()) {
            qDebug() << "Address of server to join  :" << m_serverToJoin;
        }
        qDebug() << "<> Paths set.";
    }

    if (m_liveCheck) {
        QFile check(liveCheckFile);
        if (check.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            auto payload = appID.toString().toUtf8();
            if (check.write(payload) == payload.size()) {
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
        m_settings.reset(new INISettingsObject({ BuildConfig.LAUNCHER_CONFIGFILE, "polymc.cfg", "multimc.cfg" }, this));

        // Theming
        m_settings->registerSetting("IconTheme", QString());
        m_settings->registerSetting("ApplicationTheme", QString());
        m_settings->registerSetting("BackgroundCat", QString("kitteh"));

        // Remembered state
        m_settings->registerSetting("LastUsedGroupForNewInstance", QString());

        m_settings->registerSetting("MenuBarInsteadOfToolBar", false);

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

        m_settings->registerSetting("ConsoleFont", resolvedDefaultMonospace);
        m_settings->registerSetting("ConsoleFontSize", defaultSize);
        m_settings->registerSetting("ConsoleMaxLines", 100000);
        m_settings->registerSetting("ConsoleOverflowStop", true);

        // Folders
        m_settings->registerSetting("InstanceDir", "instances");
        m_settings->registerSetting({ "CentralModsDir", "ModsDir" }, "mods");
        m_settings->registerSetting("IconsDir", "icons");
        m_settings->registerSetting("DownloadsDir", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        m_settings->registerSetting("DownloadsDirWatchRecursive", false);

        // Editors
        m_settings->registerSetting("JsonEditor", QString());

        // Language
        m_settings->registerSetting("Language", QString());
        m_settings->registerSetting("UseSystemLocale", false);

        // Console
        m_settings->registerSetting("ShowConsole", false);
        m_settings->registerSetting("AutoCloseConsole", false);
        m_settings->registerSetting("ShowConsoleOnError", true);
        m_settings->registerSetting("LogPrePostOutput", true);

        // Window Size
        m_settings->registerSetting({ "LaunchMaximized", "MCWindowMaximize" }, false);
        m_settings->registerSetting({ "MinecraftWinWidth", "MCWindowWidth" }, 854);
        m_settings->registerSetting({ "MinecraftWinHeight", "MCWindowHeight" }, 480);

        // Proxy Settings
        m_settings->registerSetting("ProxyType", "None");
        m_settings->registerSetting({ "ProxyAddr", "ProxyHostName" }, "127.0.0.1");
        m_settings->registerSetting("ProxyPort", 8080);
        m_settings->registerSetting({ "ProxyUser", "ProxyUsername" }, "");
        m_settings->registerSetting({ "ProxyPass", "ProxyPassword" }, "");

        // Memory
        m_settings->registerSetting({ "MinMemAlloc", "MinMemoryAlloc" }, 512);
        m_settings->registerSetting({ "MaxMemAlloc", "MaxMemoryAlloc" }, suitableMaxMem());
        m_settings->registerSetting("PermGen", 128);

        // Java Settings
        m_settings->registerSetting("JavaPath", "");
        m_settings->registerSetting("JavaSignature", "");
        m_settings->registerSetting("JavaArchitecture", "");
        m_settings->registerSetting("JavaRealArchitecture", "");
        m_settings->registerSetting("JavaVersion", "");
        m_settings->registerSetting("JavaVendor", "");
        m_settings->registerSetting("LastHostname", "");
        m_settings->registerSetting("JvmArgs", "");
        m_settings->registerSetting("IgnoreJavaCompatibility", false);
        m_settings->registerSetting("IgnoreJavaWizard", false);

        // Native library workarounds
        m_settings->registerSetting("UseNativeOpenAL", false);
        m_settings->registerSetting("CustomOpenALPath", "");
        m_settings->registerSetting("UseNativeGLFW", false);
        m_settings->registerSetting("CustomGLFWPath", "");

        // Peformance related options
        m_settings->registerSetting("EnableFeralGamemode", false);
        m_settings->registerSetting("EnableMangoHud", false);
        m_settings->registerSetting("UseDiscreteGpu", false);

        // Game time
        m_settings->registerSetting("ShowGameTime", true);
        m_settings->registerSetting("ShowGlobalGameTime", true);
        m_settings->registerSetting("RecordGameTime", true);
        m_settings->registerSetting("ShowGameTimeWithoutDays", false);

        // Minecraft mods
        m_settings->registerSetting("ModMetadataDisabled", false);

        // Minecraft offline player name
        m_settings->registerSetting("LastOfflinePlayerName", "");

        // Wrapper command for launch
        m_settings->registerSetting("WrapperCommand", "");

        // Custom Commands
        m_settings->registerSetting({ "PreLaunchCommand", "PreLaunchCmd" }, "");
        m_settings->registerSetting({ "PostExitCommand", "PostExitCmd" }, "");

        // The cat
        m_settings->registerSetting("TheCat", false);

        m_settings->registerSetting("ToolbarsLocked", false);

        m_settings->registerSetting("InstSortMode", "Name");
        m_settings->registerSetting("SelectedInstance", QString());

        // Window state and geometry
        m_settings->registerSetting("MainWindowState", "");
        m_settings->registerSetting("MainWindowGeometry", "");

        m_settings->registerSetting("ConsoleWindowState", "");
        m_settings->registerSetting("ConsoleWindowGeometry", "");

        m_settings->registerSetting("SettingsGeometry", "");

        m_settings->registerSetting("PagedGeometry", "");

        m_settings->registerSetting("NewInstanceGeometry", "");

        m_settings->registerSetting("UpdateDialogGeometry", "");

        m_settings->registerSetting("ModDownloadGeometry", "");
        m_settings->registerSetting("RPDownloadGeometry", "");
        m_settings->registerSetting("TPDownloadGeometry", "");
        m_settings->registerSetting("ShaderDownloadGeometry", "");

        // HACK: This code feels so stupid is there a less stupid way of doing this?
        {
            m_settings->registerSetting("PastebinURL", "");
            m_settings->registerSetting("PastebinType", PasteUpload::PasteType::Mclogs);
            m_settings->registerSetting("PastebinCustomAPIBase", "");

            QString pastebinURL = m_settings->get("PastebinURL").toString();

            bool userHadDefaultPastebin = pastebinURL == "https://0x0.st";
            if (!pastebinURL.isEmpty() && !userHadDefaultPastebin) {
                m_settings->set("PastebinType", PasteUpload::PasteType::NullPointer);
                m_settings->set("PastebinCustomAPIBase", pastebinURL);
                m_settings->reset("PastebinURL");
            }

            bool ok;
            int pasteType = m_settings->get("PastebinType").toInt(&ok);
            // If PastebinType is invalid then reset the related settings.
            if (!ok || !(PasteUpload::PasteType::First <= pasteType && pasteType <= PasteUpload::PasteType::Last)) {
                m_settings->reset("PastebinType");
                m_settings->reset("PastebinCustomAPIBase");
            }
        }
        {
            // Meta URL
            m_settings->registerSetting("MetaURLOverride", "");

            QUrl metaUrl(m_settings->get("MetaURLOverride").toString());

            // get rid of invalid meta urls
            if (!metaUrl.isValid() || (metaUrl.scheme() != "http" && metaUrl.scheme() != "https"))
                m_settings->reset("MetaURLOverride");
        }

        m_settings->registerSetting("CloseAfterLaunch", false);
        m_settings->registerSetting("QuitAfterGameStop", false);

        // Custom Microsoft Authentication Client ID
        m_settings->registerSetting("MSAClientIDOverride", "");

        // Custom Flame API Key
        {
            m_settings->registerSetting("CFKeyOverride", "");
            m_settings->registerSetting("FlameKeyOverride", "");

            QString flameKey = m_settings->get("CFKeyOverride").toString();

            if (!flameKey.isEmpty())
                m_settings->set("FlameKeyOverride", flameKey);
            m_settings->reset("CFKeyOverride");
        }
        m_settings->registerSetting("ModrinthToken", "");
        m_settings->registerSetting("UserAgentOverride", "");

        // Init page provider
        {
            m_globalSettingsProvider = std::make_shared<GenericPageProvider>(tr("Settings"));
            m_globalSettingsProvider->addPage<LauncherPage>();
            m_globalSettingsProvider->addPage<MinecraftPage>();
            m_globalSettingsProvider->addPage<JavaPage>();
            m_globalSettingsProvider->addPage<LanguagePage>();
            m_globalSettingsProvider->addPage<CustomCommandsPage>();
            m_globalSettingsProvider->addPage<ProxyPage>();
            m_globalSettingsProvider->addPage<ExternalToolsPage>();
            m_globalSettingsProvider->addPage<AccountListPage>();
            m_globalSettingsProvider->addPage<APIPage>();
        }

        PixmapCache::setInstance(new PixmapCache(this));

        qDebug() << "<> Settings loaded.";
    }

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installFactory(groupViewAccessibleFactory);
#endif /* !QT_NO_ACCESSIBILITY */

    // initialize network access and proxy setup
    {
        m_network.reset(new QNetworkAccessManager());
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
        m_translations.reset(new TranslationsModel("translations"));
        auto bcp47Name = m_settings->get("Language").toString();
        m_translations->selectLanguage(bcp47Name);
        qDebug() << "Your language is" << bcp47Name;
        qDebug() << "<> Translations loaded.";
    }

    // initialize the updater
    if (BuildConfig.UPDATER_ENABLED) {
        qDebug() << "Initializing updater";
#if defined(Q_OS_MAC) && defined(SPARKLE_ENABLED)
        m_updater.reset(new MacSparkleUpdater());
#endif
        qDebug() << "<> Updater started.";
    }

    // Instance icons
    {
        auto setting = APPLICATION->settings()->getSetting("IconsDir");
        QStringList instFolders = { ":/icons/multimc/32x32/instances/", ":/icons/multimc/50x50/instances/",
                                    ":/icons/multimc/128x128/instances/", ":/icons/multimc/scalable/instances/" };
        m_icons.reset(new IconList(instFolders, setting->get().toString()));
        connect(setting.get(), &Setting::SettingChanged,
                [&](const Setting&, QVariant value) { m_icons->directoryChanged(value.toString()); });
        qDebug() << "<> Instance icons intialized.";
    }

    // Themes
    m_themeManager = std::make_unique<ThemeManager>();

    // initialize and load all instances
    {
        auto InstDirSetting = m_settings->getSetting("InstanceDir");
        // instance path: check for problems with '!' in instance path and warn the user in the log
        // and remember that we have to show him a dialog when the gui starts (if it does so)
        QString instDir = InstDirSetting->get().toString();
        qDebug() << "Instance path              : " << instDir;
        if (FS::checkProblemticPathJava(QDir(instDir))) {
            qWarning() << "Your instance path contains \'!\' and this is known to cause java problems!";
        }
        m_instances.reset(new InstanceList(m_settings, instDir, this));
        connect(InstDirSetting.get(), &Setting::SettingChanged, m_instances.get(), &InstanceList::on_InstFolderChanged);
        qDebug() << "Loading Instances...";
        m_instances->loadList();
        qDebug() << "<> Instances loaded.";
    }

    // and accounts
    {
        m_accounts.reset(new AccountList(this));
        qDebug() << "Loading accounts...";
        m_accounts->setListFilePath("accounts.json", true);
        m_accounts->loadList();
        m_accounts->fillQueue();
        qDebug() << "<> Accounts loaded.";
    }

    // init the http meta cache
    {
        m_metacache.reset(new HttpMetaCache("metacache"));
        m_metacache->addBase("asset_indexes", QDir("assets/indexes").absolutePath());
        m_metacache->addBase("asset_objects", QDir("assets/objects").absolutePath());
        m_metacache->addBase("versions", QDir("versions").absolutePath());
        m_metacache->addBase("libraries", QDir("libraries").absolutePath());
        m_metacache->addBase("minecraftforge", QDir("mods/minecraftforge").absolutePath());
        m_metacache->addBase("fmllibs", QDir("mods/minecraftforge/libs").absolutePath());
        m_metacache->addBase("liteloader", QDir("mods/liteloader").absolutePath());
        m_metacache->addBase("general", QDir("cache").absolutePath());
        m_metacache->addBase("ATLauncherPacks", QDir("cache/ATLauncherPacks").absolutePath());
        m_metacache->addBase("FTBPacks", QDir("cache/FTBPacks").absolutePath());
        m_metacache->addBase("ModpacksCHPacks", QDir("cache/ModpacksCHPacks").absolutePath());
        m_metacache->addBase("TechnicPacks", QDir("cache/TechnicPacks").absolutePath());
        m_metacache->addBase("FlamePacks", QDir("cache/FlamePacks").absolutePath());
        m_metacache->addBase("FlameMods", QDir("cache/FlameMods").absolutePath());
        m_metacache->addBase("ModrinthPacks", QDir("cache/ModrinthPacks").absolutePath());
        m_metacache->addBase("ModrinthModpacks", QDir("cache/ModrinthModpacks").absolutePath());
        m_metacache->addBase("root", QDir::currentPath());
        m_metacache->addBase("translations", QDir("translations").absolutePath());
        m_metacache->addBase("icons", QDir("cache/icons").absolutePath());
        m_metacache->addBase("meta", QDir("meta").absolutePath());
        m_metacache->Load();
        qDebug() << "<> Cache initialized.";
    }

    // now we have network, download translation updates
    m_translations->downloadIndex();

    // FIXME: what to do with these?
    m_profilers.insert("jprofiler", std::shared_ptr<BaseProfilerFactory>(new JProfilerFactory()));
    m_profilers.insert("jvisualvm", std::shared_ptr<BaseProfilerFactory>(new JVisualVMFactory()));
    for (auto profiler : m_profilers.values()) {
        profiler->registerSettings(m_settings);
    }

    // Create the MCEdit thing... why is this here?
    {
        m_mcedit.reset(new MCEditTool(m_settings));
    }

#ifdef Q_OS_MACOS
    connect(this, &Application::clickedOnDock, [this]() { this->showMainWindow(); });
#endif

    connect(this, &Application::aboutToQuit, [this]() {
        if (m_instances) {
            // save any remaining instance state
            m_instances->saveNow();
        }
        if (logFile) {
            logFile->flush();
            logFile->close();
        }
    });

    updateCapabilities();

    detectLibraries();

    if (createSetupWizard()) {
        return;
    }

    m_themeManager->applyCurrentlySelectedTheme(true);
    performMainStartupAction();
}

bool Application::createSetupWizard()
{
    bool javaRequired = [&]() {
        bool ignoreJavaWizard = m_settings->get("IgnoreJavaWizard").toBool();
        if (ignoreJavaWizard) {
            return false;
        }
        QString currentHostName = QHostInfo::localHostName();
        QString oldHostName = settings()->get("LastHostname").toString();
        if (currentHostName != oldHostName) {
            settings()->set("LastHostname", currentHostName);
            return true;
        }
        QString currentJavaPath = settings()->get("JavaPath").toString();
        QString actualPath = FS::ResolveExecutable(currentJavaPath);
        if (actualPath.isNull()) {
            return true;
        }
        return false;
    }();
    bool languageRequired = settings()->get("Language").toString().isEmpty();
    bool pasteInterventionRequired = settings()->get("PastebinURL") != "";
    bool validWidgets = m_themeManager->isValidApplicationTheme(settings()->get("ApplicationTheme").toString());
    bool validIcons = m_themeManager->isValidIconTheme(settings()->get("IconTheme").toString());
    bool themeInterventionRequired = !validWidgets || !validIcons;
    bool wizardRequired = javaRequired || languageRequired || pasteInterventionRequired || themeInterventionRequired;

    if (wizardRequired) {
        // set default theme after going into theme wizard
        if (!validIcons)
            settings()->set("IconTheme", QString("pe_colored"));
        if (!validWidgets)
            settings()->set("ApplicationTheme", QString("system"));

        m_themeManager->applyCurrentlySelectedTheme(true);

        m_setupWizard = new SetupWizard(nullptr);
        if (languageRequired) {
            m_setupWizard->addPage(new LanguageWizardPage(m_setupWizard));
        }

        if (javaRequired) {
            m_setupWizard->addPage(new JavaWizardPage(m_setupWizard));
        }

        if (pasteInterventionRequired) {
            m_setupWizard->addPage(new PasteWizardPage(m_setupWizard));
        }

        if (themeInterventionRequired) {
            m_setupWizard->addPage(new ThemeWizardPage(m_setupWizard));
        }

        connect(m_setupWizard, &QDialog::finished, this, &Application::setupWizardFinished);
        m_setupWizard->show();
        return true;
    }
    return false;
}

bool Application::event(QEvent* event)
{
#ifdef Q_OS_MACOS
    if (event->type() == QEvent::ApplicationStateChange) {
        auto ev = static_cast<QApplicationStateChangeEvent*>(event);

        if (m_prevAppState == Qt::ApplicationActive && ev->applicationState() == Qt::ApplicationActive) {
            emit clickedOnDock();
        }
        m_prevAppState = ev->applicationState();
    }
#endif

    if (event->type() == QEvent::FileOpen) {
        auto ev = static_cast<QFileOpenEvent*>(event);
        m_mainWindow->processURLs({ ev->url() });
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
    m_status = Application::Initialized;
    if (!m_instanceIdToLaunch.isEmpty()) {
        auto inst = instances()->getInstanceById(m_instanceIdToLaunch);
        if (inst) {
            MinecraftServerTargetPtr serverToJoin = nullptr;
            MinecraftAccountPtr accountToUse = nullptr;

            qDebug() << "<> Instance" << m_instanceIdToLaunch << "launching";
            if (!m_serverToJoin.isEmpty()) {
                // FIXME: validate the server string
                serverToJoin.reset(new MinecraftServerTarget(MinecraftServerTarget::parse(m_serverToJoin)));
                qDebug() << "   Launching with server" << m_serverToJoin;
            }

            if (!m_profileToUse.isEmpty()) {
                accountToUse = accounts()->getAccountByProfileName(m_profileToUse);
                if (!accountToUse) {
                    return;
                }
                qDebug() << "   Launching with account" << m_profileToUse;
            }

            launch(inst, true, false, serverToJoin, accountToUse);
            return;
        }
    }
    if (!m_instanceIdToShowWindowOf.isEmpty()) {
        auto inst = instances()->getInstanceById(m_instanceIdToShowWindowOf);
        if (inst) {
            qDebug() << "<> Showing window of instance " << m_instanceIdToShowWindowOf;
            showInstanceWindow(inst);
            return;
        }
    }
    if (!m_mainWindow) {
        // normal main window
        showMainWindow(false);
        qDebug() << "<> Main window shown.";
    }
    if (!m_urlsToImport.isEmpty()) {
        qDebug() << "<> Importing from url:" << m_urlsToImport;
        m_mainWindow->processURLs(m_urlsToImport);
    }
}

void Application::showFatalErrorMessage(const QString& title, const QString& content)
{
    m_status = Application::Failed;
    auto dialog = CustomMessageBox::selectable(nullptr, title, content, QMessageBox::Critical);
    dialog->exec();
}

Application::~Application()
{
    // Shut down logger by setting the logger function to nothing
    qInstallMessageHandler(nullptr);

#if defined Q_OS_WIN32
    // Detach from Windows console
    if (consoleAttached) {
        fclose(stdout);
        fclose(stdin);
        fclose(stderr);
        FreeConsole();
    }
#endif
}

void Application::messageReceived(const QByteArray& message)
{
    if (status() != Initialized) {
        qDebug() << "Received message" << message << "while still initializing. It will be ignored.";
        return;
    }

    ApplicationMessage received;
    received.parse(message);

    auto& command = received.command;

    if (command == "activate") {
        showMainWindow();
    } else if (command == "import") {
        QString url = received.args["url"];
        if (url.isEmpty()) {
            qWarning() << "Received" << command << "message without a zip path/URL.";
            return;
        }
        m_mainWindow->processURLs({ normalizeImportUrl(url) });
    } else if (command == "launch") {
        QString id = received.args["id"];
        QString server = received.args["server"];
        QString profile = received.args["profile"];

        InstancePtr instance;
        if (!id.isEmpty()) {
            instance = instances()->getInstanceById(id);
            if (!instance) {
                qWarning() << "Launch command requires an valid instance ID. " << id << "resolves to nothing.";
                return;
            }
        } else {
            qWarning() << "Launch command called without an instance ID...";
            return;
        }

        MinecraftServerTargetPtr serverObject = nullptr;
        if (!server.isEmpty()) {
            serverObject = std::make_shared<MinecraftServerTarget>(MinecraftServerTarget::parse(server));
        }

        MinecraftAccountPtr accountObject;
        if (!profile.isEmpty()) {
            accountObject = accounts()->getAccountByProfileName(profile);
            if (!accountObject) {
                qWarning() << "Launch command requires the specified profile to be valid. " << profile
                           << "does not resolve to any account.";
                return;
            }
        }

        launch(instance, true, false, serverObject, accountObject);
    } else {
        qWarning() << "Received invalid message" << message;
    }
}

std::shared_ptr<TranslationsModel> Application::translations()
{
    return m_translations;
}

std::shared_ptr<JavaInstallList> Application::javalist()
{
    if (!m_javalist) {
        m_javalist.reset(new JavaInstallList());
    }
    return m_javalist;
}

QIcon Application::getThemedIcon(const QString& name)
{
    if (name == "logo") {
        return QIcon(":/" + BuildConfig.LAUNCHER_SVGFILENAME);
    }
    return QIcon::fromTheme(name);
}

bool Application::openJsonEditor(const QString& filename)
{
    const QString file = QDir::current().absoluteFilePath(filename);
    if (m_settings->get("JsonEditor").toString().isEmpty()) {
        return DesktopServices::openUrl(QUrl::fromLocalFile(file));
    } else {
        // return DesktopServices::openFile(m_settings->get("JsonEditor").toString(), file);
        return DesktopServices::run(m_settings->get("JsonEditor").toString(), { file });
    }
}

bool Application::launch(InstancePtr instance,
                         bool online,
                         bool demo,
                         MinecraftServerTargetPtr serverToJoin,
                         MinecraftAccountPtr accountToUse)
{
    if (m_updateRunning) {
        qDebug() << "Cannot launch instances while an update is running. Please try again when updates are completed.";
    } else if (instance->canLaunch()) {
        auto& extras = m_instanceExtras[instance->id()];
        auto window = extras.window;
        if (window) {
            if (!window->saveAll()) {
                return false;
            }
        }
        auto& controller = extras.controller;
        controller.reset(new LaunchController());
        controller->setInstance(instance);
        controller->setOnline(online);
        controller->setDemo(demo);
        controller->setProfiler(profilers().value(instance->settings()->get("Profiler").toString(), nullptr).get());
        controller->setServerToJoin(serverToJoin);
        controller->setAccountToUse(accountToUse);
        if (window) {
            controller->setParentWidget(window);
        } else if (m_mainWindow) {
            controller->setParentWidget(m_mainWindow);
        }
        connect(controller.get(), &LaunchController::succeeded, this, &Application::controllerSucceeded);
        connect(controller.get(), &LaunchController::failed, this, &Application::controllerFailed);
        connect(controller.get(), &LaunchController::aborted, this, [this] { controllerFailed(tr("Aborted")); });
        addRunningInstance();
        controller->start();
        return true;
    } else if (instance->isRunning()) {
        showInstanceWindow(instance, "console");
        return true;
    } else if (instance->canEdit()) {
        showInstanceWindow(instance);
        return true;
    }
    return false;
}

bool Application::kill(InstancePtr instance)
{
    if (!instance->isRunning()) {
        qWarning() << "Attempted to kill instance" << instance->id() << ", which isn't running.";
        return false;
    }
    auto& extras = m_instanceExtras[instance->id()];
    // NOTE: copy of the shared pointer keeps it alive
    auto controller = extras.controller;
    if (controller) {
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
    m_runningInstances++;
    if (m_runningInstances == 1) {
        emit updateAllowedChanged(false);
    }
}

void Application::subRunningInstance()
{
    if (m_runningInstances == 0) {
        qCritical() << "Something went really wrong and we now have less than 0 running instances... WTF";
        return;
    }
    m_runningInstances--;
    if (m_runningInstances == 0) {
        emit updateAllowedChanged(true);
    }
}

bool Application::shouldExitNow() const
{
    return m_runningInstances == 0 && m_openWindows == 0;
}

bool Application::updatesAreAllowed()
{
    return m_runningInstances == 0;
}

void Application::updateIsRunning(bool running)
{
    m_updateRunning = running;
}

void Application::controllerSucceeded()
{
    auto controller = qobject_cast<LaunchController*>(QObject::sender());
    if (!controller)
        return;
    auto id = controller->id();
    auto& extras = m_instanceExtras[id];

    // on success, do...
    if (controller->instance()->settings()->get("AutoCloseConsole").toBool()) {
        if (extras.window) {
            extras.window->close();
        }
    }
    extras.controller.reset();
    subRunningInstance();

    // quit when there are no more windows.
    if (shouldExitNow()) {
        m_status = Status::Succeeded;
        exit(0);
    }
}

void Application::controllerFailed(const QString& error)
{
    Q_UNUSED(error);
    auto controller = qobject_cast<LaunchController*>(QObject::sender());
    if (!controller)
        return;
    auto id = controller->id();
    auto& extras = m_instanceExtras[id];

    // on failure, do... nothing
    extras.controller.reset();
    subRunningInstance();

    // quit when there are no more windows.
    if (shouldExitNow()) {
        m_status = Status::Failed;
        exit(1);
    }
}

void Application::ShowGlobalSettings(class QWidget* parent, QString open_page)
{
    if (!m_globalSettingsProvider) {
        return;
    }
    emit globalSettingsAboutToOpen();
    {
        SettingsObject::Lock lock(APPLICATION->settings());
        PageDialog dlg(m_globalSettingsProvider.get(), open_page, parent);
        dlg.exec();
    }
    emit globalSettingsClosed();
}

MainWindow* Application::showMainWindow(bool minimized)
{
    if (m_mainWindow) {
        m_mainWindow->setWindowState(m_mainWindow->windowState() & ~Qt::WindowMinimized);
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    } else {
        m_mainWindow = new MainWindow();
        m_mainWindow->restoreState(QByteArray::fromBase64(APPLICATION->settings()->get("MainWindowState").toByteArray()));
        m_mainWindow->restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("MainWindowGeometry").toByteArray()));

        if (minimized) {
            m_mainWindow->showMinimized();
        } else {
            m_mainWindow->show();
        }

        m_mainWindow->checkInstancePathForProblems();
        connect(this, &Application::updateAllowedChanged, m_mainWindow, &MainWindow::updatesAllowedChanged);
        connect(m_mainWindow, &MainWindow::isClosing, this, &Application::on_windowClose);
        m_openWindows++;
    }
    return m_mainWindow;
}

InstanceWindow* Application::showInstanceWindow(InstancePtr instance, QString page)
{
    if (!instance)
        return nullptr;
    auto id = instance->id();
    auto& extras = m_instanceExtras[id];
    auto& window = extras.window;

    if (window) {
        window->raise();
        window->activateWindow();
    } else {
        window = new InstanceWindow(instance);
        m_openWindows++;
        connect(window, &InstanceWindow::isClosing, this, &Application::on_windowClose);
    }
    if (!page.isEmpty()) {
        window->selectPage(page);
    }
    if (extras.controller) {
        extras.controller->setParentWidget(window);
    }
    return window;
}

void Application::on_windowClose()
{
    m_openWindows--;
    auto instWindow = qobject_cast<InstanceWindow*>(QObject::sender());
    if (instWindow) {
        auto& extras = m_instanceExtras[instWindow->instanceId()];
        extras.window = nullptr;
        if (extras.controller) {
            extras.controller->setParentWidget(m_mainWindow);
        }
    }
    auto mainWindow = qobject_cast<MainWindow*>(QObject::sender());
    if (mainWindow) {
        m_mainWindow = nullptr;
    }
    // quit when there are no more windows.
    if (shouldExitNow()) {
        exit(0);
    }
}

void Application::updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password)
{
    // Set the application proxy settings.
    if (proxyTypeStr == "SOCKS5") {
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, addr, port, user, password));
    } else if (proxyTypeStr == "HTTP") {
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, addr, port, user, password));
    } else if (proxyTypeStr == "None") {
        // If we have no proxy set, set no proxy and return.
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    } else {
        // If we have "Default" selected, set Qt to use the system proxy settings.
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    }

    qDebug() << "Detecting proxy settings...";
    QNetworkProxy proxy = QNetworkProxy::applicationProxy();
    m_network->setProxy(proxy);

    QString proxyDesc;
    if (proxy.type() == QNetworkProxy::NoProxy) {
        qDebug() << "Using no proxy is an option!";
        return;
    }
    switch (proxy.type()) {
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
    proxyDesc += QString("%1:%2").arg(proxy.hostName()).arg(proxy.port());
    qDebug() << proxyDesc;
}

shared_qobject_ptr<HttpMetaCache> Application::metacache()
{
    return m_metacache;
}

shared_qobject_ptr<QNetworkAccessManager> Application::network()
{
    return m_network;
}

shared_qobject_ptr<Meta::Index> Application::metadataIndex()
{
    if (!m_metadataIndex) {
        m_metadataIndex.reset(new Meta::Index());
    }
    return m_metadataIndex;
}

void Application::updateCapabilities()
{
    m_capabilities = None;
    if (!getMSAClientID().isEmpty())
        m_capabilities |= SupportsMSA;
    if (!getFlameAPIKey().isEmpty())
        m_capabilities |= SupportsFlame;

#ifdef Q_OS_LINUX
    if (gamemode_query_status() >= 0)
        m_capabilities |= SupportsGameMode;

    if (!MangoHud::getLibraryString().isEmpty())
        m_capabilities |= SupportsMangoHud;
#endif
}

void Application::detectLibraries()
{
#ifdef Q_OS_LINUX
    m_detectedGLFWPath = MangoHud::findLibrary(BuildConfig.GLFW_LIBRARY_NAME);
    m_detectedOpenALPath = MangoHud::findLibrary(BuildConfig.OPENAL_LIBRARY_NAME);
    qDebug() << "Detected native libraries:" << m_detectedGLFWPath << m_detectedOpenALPath;
#endif
}

QString Application::getJarPath(QString jarFile)
{
    QStringList potentialPaths = {
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        FS::PathCombine(m_rootPath, "share", BuildConfig.LAUNCHER_NAME),
#endif
        FS::PathCombine(m_rootPath, "jars"),
        FS::PathCombine(applicationDirPath(), "jars"),
        FS::PathCombine(applicationDirPath(), "..", "jars")  // from inside build dir, for debuging
    };
    for (QString p : potentialPaths) {
        QString jarPath = FS::PathCombine(p, jarFile);
        if (QFileInfo(jarPath).isFile())
            return jarPath;
    }
    return {};
}

QString Application::getMSAClientID()
{
    QString clientIDOverride = m_settings->get("MSAClientIDOverride").toString();
    if (!clientIDOverride.isEmpty()) {
        return clientIDOverride;
    }

    return BuildConfig.MSA_CLIENT_ID;
}

QString Application::getFlameAPIKey()
{
    QString keyOverride = m_settings->get("FlameKeyOverride").toString();
    if (!keyOverride.isEmpty()) {
        return keyOverride;
    }

    return BuildConfig.FLAME_API_KEY;
}

QString Application::getModrinthAPIToken()
{
    QString tokenOverride = m_settings->get("ModrinthToken").toString();
    if (!tokenOverride.isEmpty())
        return tokenOverride;

    return QString();
}

QString Application::getUserAgent()
{
    QString uaOverride = m_settings->get("UserAgentOverride").toString();
    if (!uaOverride.isEmpty()) {
        return uaOverride.replace("$LAUNCHER_VER", BuildConfig.printableVersionString());
    }

    return BuildConfig.USER_AGENT;
}

QString Application::getUserAgentUncached()
{
    QString uaOverride = m_settings->get("UserAgentOverride").toString();
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
        maxMemoryAlloc = (int)(totalRAM / 1.5);
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
        matcher->add(std::make_shared<SimplePrefixMatcher>("logs/"));
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

void Application::triggerUpdateCheck()
{
    if (m_updater) {
        qDebug() << "Checking for updates.";
        m_updater->setBetaAllowed(false);  // There are no other channels than stable
        m_updater->checkForUpdates();
    } else {
        qDebug() << "Updater not available.";
    }
}

QUrl Application::normalizeImportUrl(QString const& url)
{
    auto local_file = QFileInfo(url);
    if (local_file.exists()) {
        return QUrl::fromLocalFile(local_file.absoluteFilePath());
    } else {
        return QUrl::fromUserInput(url);
    }
}
