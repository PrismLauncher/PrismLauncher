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
#include "java/JavaInstallList.h"
#include "net/PasteUpload.h"
#include "tasks/Task.h"
#include "tools/GenericProfiler.h"
#include "ui/InstanceWindow.h"
#include "ui/MainWindow.h"
#include "ui/ToolTipFilter.h"
#include "ui/ViewLogWindow.h"

#include "ui/dialogs/ProgressDialog.h"
#include "ui/instanceview/AccessibleInstanceView.h"

#include "ui/pages/BasePageProvider.h"
#include "ui/pages/global/APIPage.h"
#include "ui/pages/global/AccountListPage.h"
#include "ui/pages/global/AppearancePage.h"
#include "ui/pages/global/ExternalToolsPage.h"
#include "ui/pages/global/JavaPage.h"
#include "ui/pages/global/LanguagePage.h"
#include "ui/pages/global/LauncherPage.h"
#include "ui/pages/global/MinecraftPage.h"
#include "ui/pages/global/ProxyPage.h"

#include "ui/setupwizard/AutoJavaWizardPage.h"
#include "ui/setupwizard/JavaWizardPage.h"
#include "ui/setupwizard/LanguageWizardPage.h"
#include "ui/setupwizard/LoginWizardPage.h"
#include "ui/setupwizard/PasteWizardPage.h"
#include "ui/setupwizard/SetupWizard.h"
#include "ui/setupwizard/ThemeWizardPage.h"

#include "ui/dialogs/CustomMessageBox.h"

#include "ui/pagedialog/PageDialog.h"

#include "ui/themes/ThemeManager.h"

#include "ApplicationMessage.h"

#include <algorithm>
#include <iostream>
#include <memory>
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
#include <QStringLiteral>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTranslator>
#include <QWindow>
#include <utility>

#include "InstanceList.h"
#include "MTPixmapCache.h"

#include <minecraft/auth/AccountList.h>
#include "icons/IconList.h"
#include "net/HttpMetaCache.h"

#include "updater/ExternalUpdater.h"

#include "tools/JProfiler.h"
#include "tools/JVisualVM.h"

#include "settings/INISettingsObject.h"
#include "settings/Setting.h"

#include "meta/Index.h"
#include "translations/TranslationsModel.h"

#include <DesktopServices.h>
#include <FileSystem.h>
#include <LocalPeer.h>

#include <stdlib.h>
#include "SysInfo.h"

#ifdef Q_OS_LINUX
#include <dlfcn.h>
#include "LibraryUtils.h"
#endif

#if defined(Q_OS_LINUX) && defined(ENABLE_GAMEMODE)
#include "gamemode_client.h"
#endif

#if defined(Q_OS_LINUX)
#include <sys/statvfs.h>
#endif

#if defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
#include <sys/mount.h>
#include <sys/types.h>
#endif

#if defined(Q_OS_MAC)
#if defined(SPARKLE_ENABLED)
#include "updater/MacSparkleUpdater.h"
#endif
#else
#include "updater/PrismExternalUpdater.h"
#endif

#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "console/Console.h"

static const QLatin1String g_liveCheckFile("live.check");

PixmapCache* PixmapCache::s_instance = nullptr;

static bool isANSIColorConsole;

static const QString g_defaultLogFormat = QStringLiteral(
    "%{time process}"
    " "
    "%{if-debug}Debug:%{endif}"
    "%{if-info}Info:%{endif}"
    "%{if-warning}Warning:%{endif}"
    "%{if-critical}Critical:%{endif}"
    "%{if-fatal}Fatal:%{endif}"
    " "
    "%{if-category}[%{category}] %{endif}"
    "%{message}"
    " "
    "(%{function}:%{line})");

#define ansi_reset "\x1b[0m"
#define ansi_bold "\x1b[1m"
#define ansi_reset_bold "\x1b[22m"
#define ansi_faint "\x1b[2m"
#define ansi_italic "\x1b[3m"
#define ansi_red_fg "\x1b[31m"
#define ansi_green_fg "\x1b[32m"
#define ansi_yellow_fg "\x1b[33m"
#define ansi_blue_fg "\x1b[34m"
#define ansi_purple_fg "\x1b[35m"
#define ansi_inverse "\x1b[7m"

struct Application::Impl {
    QHash<QString, int> qsaveResources;
    mutable QMutex qsaveResourcesMutex;

    std::unique_ptr<QNetworkAccessManager> network;

    std::unique_ptr<ExternalUpdater> updater;
    std::unique_ptr<AccountList> accounts;

    std::unique_ptr<HttpMetaCache> metacache;
    std::unique_ptr<Meta::Index> metadataIndex;

    std::unique_ptr<SettingsObject> settings;
    std::unique_ptr<SettingsObject> playtimeSettings;
    std::unique_ptr<InstanceList> instances;
    std::unique_ptr<IconList> icons;
    std::unique_ptr<JavaInstallList> javalist;
    std::unique_ptr<TranslationsModel> translations;
    std::unique_ptr<GenericPageProvider> globalSettingsProvider;
    std::unique_ptr<ThemeManager> themeManager;

    QMap<QString, std::shared_ptr<BaseProfilerFactory>> profilers;

    QString rootPath;
    QString dataPath;
    bool initialized = false;
    Capabilities capabilities;
    bool portable = false;

#ifdef Q_OS_MACOS
    Qt::ApplicationState prevAppState = Qt::ApplicationInactive;
#endif

    // main state variables
    size_t openWindows = 0;
    size_t runningInstances = 0;
    bool updateRunning = false;

    // main window, if any
    MainWindow* mainWindow = nullptr;

    // log window, if any
    ViewLogWindow* viewLogWindow = nullptr;

    // peer launcher instance connector - used to implement single instance launcher and signalling
    LocalPeer* peerInstance = nullptr;

    QString detectedGLFWPath;
    QString detectedOpenALPath;
    QString detectedSDLPath;
    QString instanceIdToLaunch;
    QString serverToJoin;
    QString worldToJoin;
    QString profileToUse;
    bool launchOffline = false;
    QString offlineName;
    bool liveCheck = false;
    QList<QUrl> urlsToImport;
    QString instanceIdToShowWindowOf;
    bool showMainWindow = false;
    std::unique_ptr<QFile> logFile;
    std::unique_ptr<LogModel> logModel;
};

// clang-format off
static const QString g_ansiLogFormat = QStringLiteral(
    ansi_faint "%{time process}" ansi_reset
    " "
    "%{if-debug}" ansi_bold ansi_green_fg "D:" ansi_reset "%{endif}"
    "%{if-info}" ansi_bold ansi_blue_fg "I:" ansi_reset "%{endif}"
    "%{if-warning}" ansi_bold ansi_yellow_fg "W:" ansi_reset_bold "%{endif}"
    "%{if-critical}" ansi_bold ansi_red_fg "C:" ansi_reset_bold "%{endif}"
    "%{if-fatal}" ansi_bold ansi_inverse ansi_red_fg "F:" ansi_reset_bold "%{endif}"
    " "
    "%{if-category}" ansi_bold "[%{category}]" ansi_reset_bold " %{endif}"
    "%{message}"
    " "
    ansi_reset ansi_faint "(%{function}:%{line})" ansi_reset
);
// clang-format on

#undef ansi_inverse
#undef ansi_purple_fg
#undef ansi_blue_fg
#undef ansi_yellow_fg
#undef ansi_green_fg
#undef ansi_red_fg
#undef ansi_italic
#undef ansi_faint
#undef ansi_bold
#undef ansi_reset_bold
#undef ansi_reset

namespace {

/** This is used so that we can output to the log file in addition to the CLI. */
void appDebugOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static std::mutex s_loggerMutex;
    const std::lock_guard<std::mutex> lock(s_loggerMutex);  // synchronized, QFile logFile is not thread-safe

    if (isANSIColorConsole) {
        // ensure default is set for log file
        qSetMessagePattern(g_defaultLogFormat);
    }

    QString out = qFormatLogMessage(type, context, msg);
    if (APPLICATION->logModel()) {
        APPLICATION->logModel()->append(MessageLevel::fromQtMsgType(type), out);
    }

    out += QChar::LineFeed;
    APPLICATION->logFile()->write(out.toUtf8());
    APPLICATION->logFile()->flush();

    if (isANSIColorConsole) {
        // format ansi for console;
        qSetMessagePattern(g_ansiLogFormat);
        out = qFormatLogMessage(type, context, msg);
        out += QChar::LineFeed;
    }

    QTextStream(stderr) << out.toLocal8Bit();
    fflush(stderr);
}

std::tuple<QDateTime, QString, QString, QString, QString> readLockFile(const QString& path)
{
    auto res = FS::read(path);
    if (!res) {
        qFatal("Failed to read lock file: %s", res.error().toUtf8().constData());
    }
    auto contents = QString(res.value());
    auto lines = contents.split('\n');

    QDateTime timestamp;
    QString from;
    QString to;
    QString target;
    QString dataPath;
    for (const auto& line : lines) {
        auto index = line.indexOf("=");
        if (index < 0) {
            continue;
        }
        auto left = line.left(index);
        auto right = line.mid(index + 1);
        if (left.toLower() == "timestamp") {
            timestamp = QDateTime::fromString(right, Qt::ISODate);
        } else if (left.toLower() == "from") {
            from = right;
        } else if (left.toLower() == "to") {
            to = right;
        } else if (left.toLower() == "target") {
            target = right;
        } else if (left.toLower() == "data_path") {
            dataPath = right;
        }
    }
    return std::make_tuple(timestamp, from, to, target, dataPath);
}

void showFatalErrorMessage(const QString& title, const QString& content)
{
    auto* dialog = CustomMessageBox::selectable(nullptr, title, content, QMessageBox::Critical);
    dialog->exec();
}

bool handleDataMigration(const QString& currentData, const QString& oldData, const QString& name, const QString& configFile)
{
    QString nomigratePath = FS::PathCombine(currentData, name + "_nomigrate.txt");
    QStringList configPaths = { FS::PathCombine(oldData, configFile), FS::PathCombine(oldData, BuildConfig.LAUNCHER_CONFIGFILE) };

    QLocale locale;

    // Is there a valid config at the old location?
    bool configExists = false;
    for (const auto& configPath : configPaths) {
        configExists |= QFileInfo::exists(configPath);
    }

    if (!configExists || QFileInfo::exists(nomigratePath)) {
        qDebug() << "<> No migration needed from" << name;
        return false;
    }

    QString message;
    bool currentExists = QFileInfo::exists(FS::PathCombine(currentData, BuildConfig.LAUNCHER_CONFIGFILE));

    if (currentExists) {
        message = QObject::tr(
                      "Old data from %1 was found, but you already have existing data for %2. Sadly you will need to migrate yourself. Do "
                      "you want to be reminded of the pending data migration next time you start %2?")
                      .arg(name, BuildConfig.LAUNCHER_DISPLAYNAME);
    } else {
        message = QObject::tr("It looks like you used %1 before. Do you want to migrate your data to the new location of %2?")
                      .arg(name, BuildConfig.LAUNCHER_DISPLAYNAME);

        QFileInfo logInfo(FS::PathCombine(oldData, name + "-0.log"));
        if (logInfo.exists()) {
            QString lastModified = logInfo.lastModified().toString(locale.dateFormat());
            message = QObject::tr("It looks like you used %1 on %2 before. Do you want to migrate your data to the new location of %3?")
                          .arg(name, lastModified, BuildConfig.LAUNCHER_DISPLAYNAME);
        }
    }

    QMessageBox::StandardButton askMoveDialogue =
        QMessageBox::question(nullptr, BuildConfig.LAUNCHER_DISPLAYNAME, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    auto setDoNotMigrate = [&nomigratePath] {
        QFile file(nomigratePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "setDoNotMigrate failed; Failed to open file" << file.fileName() << "for writing:" << file.errorString();
        }
    };

    // create no-migrate file if user doesn't want to migrate
    if (askMoveDialogue != QMessageBox::Yes) {
        qDebug() << "<> Migration declined for" << name;
        setDoNotMigrate();
        return currentExists;  // cancel further migrations, if we already have a data directory
    }

    if (!currentExists) {
        // Migrate!
        using namespace Filters;

        QList<Filter> filters;
        filters.append(equals(configFile));
        filters.append(equals(BuildConfig.LAUNCHER_CONFIGFILE));  // it's possible that we already used that directory before
        filters.append(startsWith("logs/"));
        filters.append(equals("accounts.json"));
        filters.append(startsWith("accounts/"));
        filters.append(startsWith("assets/"));
        filters.append(startsWith("icons/"));
        filters.append(startsWith("instances/"));
        filters.append(startsWith("libraries/"));
        filters.append(startsWith("mods/"));
        filters.append(startsWith("themes/"));

        ProgressDialog diag;
        DataMigrationTask task(oldData, currentData, any(std::move(filters)));
        if (diag.execWithTask(&task) != 0) {
            qDebug() << "<> Migration succeeded";
            setDoNotMigrate();
        } else {
            QString reason = task.failReason();
            QMessageBox::critical(nullptr, BuildConfig.LAUNCHER_DISPLAYNAME, QObject::tr("Migration failed! Reason: %1").arg(reason));
        }
    } else {
        qWarning() << "<> Migration was skipped, due to existing data";
    }
    return true;
}

}  // namespace

Application::Application() : m_impl(std::make_unique<Impl>())
{
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

Application::~Application()
{
    Q_ASSERT(s_instance == this);
    s_instance = nullptr;
    // Shut down logger by setting the logger function to nothing
    qInstallMessageHandler(nullptr);
}

int Application::exec(int& argc, char** argv)
{
    QApplication app(argc, argv);
    app.installEventFilter(this);

    if (console::isConsole()) {
        isANSIColorConsole = true;
    }

    QApplication::setOrganizationName(BuildConfig.LAUNCHER_NAME);
    QApplication::setOrganizationDomain(BuildConfig.LAUNCHER_DOMAIN);
    QApplication::setApplicationName(BuildConfig.LAUNCHER_NAME);
    QApplication::setApplicationDisplayName(QString("%1 %2").arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.printableVersionString()));
    QApplication::setApplicationVersion(BuildConfig.printableVersionString() + "\n" + BuildConfig.GIT_COMMIT);
    QApplication::setDesktopFileName(BuildConfig.LAUNCHER_APPID);

    // Don't quit on hiding the last window
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setQuitLockEnabled(false);

    // Commandline parsing
    QCommandLineParser parser;
    parser.setApplicationDescription(BuildConfig.LAUNCHER_DISPLAYNAME);

    parser.addOptions(
        { { { "d", "dir" }, "Use a custom path as application root (use '.' for current directory)", "directory" },
          { { "l", "launch" }, "Launch the specified instance (by instance ID)", "instance" },
          { { "s", "server" }, "Join the specified server on launch (only valid in combination with --launch)", "address" },
          { { "w", "world" }, "Join the specified world on launch (only valid in combination with --launch)", "world" },
          { { "a", "profile" }, "Use the account specified by its profile name (only valid in combination with --launch)", "profile" },
          { { "o", "offline" }, "Launch offline, with given player name (only valid in combination with --launch)", "offline" },
          { "alive", "Write a small '" + g_liveCheckFile + "' file after the launcher starts" },
          { "show-window", "Show the main launcher window (useful in combination with --launch)" },
          { { "I", "import" }, "Import instance or resource from specified local path or URL", "url" },
          { "show", "Opens the window for the specified instance (by instance ID)", "show" } });
    // Has to be positional for some OS to handle that properly
    parser.addPositionalArgument("URL", "Import the resource(s) at the given URL(s) (same as -I / --import)", "[URL...]");

    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(QApplication::arguments());

    m_impl->instanceIdToLaunch = parser.value("launch");
    m_impl->serverToJoin = parser.value("server");
    m_impl->worldToJoin = parser.value("world");
    m_impl->profileToUse = parser.value("profile");
    if (parser.isSet("offline")) {
        m_impl->launchOffline = true;
        m_impl->offlineName = parser.value("offline");
    }
    m_impl->liveCheck = parser.isSet("alive");

    m_impl->instanceIdToShowWindowOf = parser.value("show");
    m_impl->showMainWindow = parser.isSet("show-window");

    for (const auto& url : parser.values("import")) {
        m_impl->urlsToImport.append(normalizeImportUrl(url));
    }

    // treat unspecified positional arguments as import urls
    for (const auto& url : parser.positionalArguments()) {
        m_impl->urlsToImport.append(normalizeImportUrl(url));
    }

    // error if --launch is missing with --server or --profile
    if ((!m_impl->serverToJoin.isEmpty() || !m_impl->worldToJoin.isEmpty() || !m_impl->profileToUse.isEmpty() || m_impl->launchOffline) &&
        m_impl->instanceIdToLaunch.isEmpty()) {
        std::cerr << "--server, --profile and --offline can only be used in combination with --launch!" << std::endl;
        return 1;
    }

    QString origcwdPath = QDir::currentPath();
    QString binPath = QApplication::applicationDirPath();

    {
        // Root path is used for updates and portable data
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        QDir foo(FS::PathCombine(binPath, ".."));  // typically portable-root or /usr
        m_impl->rootPath = foo.absolutePath();
#elif defined(Q_OS_WIN32)
        m_impl->rootPath = binPath;
#elif defined(Q_OS_MAC)
        QDir foo(FS::PathCombine(binPath, "../.."));
        m_impl->rootPath = foo.absolutePath();
        // on macOS, touch the root to force Finder to reload the .app metadata (and fix any icon change issues)
        FS::updateTimestamp(m_impl->rootPath);
#endif
    }

    QString adjustedBy;
    QString dataPath;
    // change folder
    QString dataDirEnv;
    QString dirParam = parser.value("dir");
    if (!dirParam.isEmpty()) {
        // the dir param. it makes multimc data path point to whatever the user specified
        // on command line
        adjustedBy = "Command line";
        dataPath = dirParam;
    } else if (dataDirEnv = QProcessEnvironment::systemEnvironment().value(QString("%1_DATA_DIR").arg(BuildConfig.LAUNCHER_NAME.toUpper()));
               !dataDirEnv.isEmpty()) {
        adjustedBy = "System environment";
        dataPath = dataDirEnv;
    } else {
        QDir foo;
        if (DesktopServices::isSnap()) {
            foo = QDir(qEnvironmentVariable("SNAP_USER_COMMON"));
        } else {
            foo = QDir(FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), ".."));
        }

        dataPath = foo.absolutePath();
        adjustedBy = "Persistent data path";

#ifndef Q_OS_MACOS
        if (auto portableUserData = FS::PathCombine(m_impl->rootPath, "UserData"); QDir(portableUserData).exists()) {
            dataPath = portableUserData;
            adjustedBy = "Portable user data path";
            m_impl->portable = true;
        } else if (QFile::exists(FS::PathCombine(m_impl->rootPath, "portable.txt"))) {
            dataPath = m_impl->rootPath;
            adjustedBy = "Portable data path";
            m_impl->portable = true;
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
        return 1;
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
        return 1;
    }
    m_impl->dataPath = dataPath;

    /*
     * Establish the mechanism for communication with an already running PrismLauncher that uses the same data path.
     * If there is one, tell it what the user actually wanted to do and exit.
     * We want to initialize this before logging to avoid messing with the log of a potential already running copy.
     */
    auto appID = ApplicationId::fromPathAndVersion(QDir::currentPath(), BuildConfig.printableVersionString());
    {
        // FIXME: you can run the same binaries with multiple data dirs and they won't clash. This could cause issues for updates.
        m_impl->peerInstance = new LocalPeer(this, appID);
        connect(m_impl->peerInstance, &LocalPeer::messageReceived, this, &Application::messageReceived);
        if (m_impl->peerInstance->isClient()) {
            bool sentMessage = false;
            int timeout = 2000;

            if (m_impl->instanceIdToLaunch.isEmpty()) {
                ApplicationMessage activate;
                activate.command = "activate";
                sentMessage = m_impl->peerInstance->sendMessage(activate.serialize(), timeout);

                if (!m_impl->urlsToImport.isEmpty()) {
                    for (const auto& url : m_impl->urlsToImport) {
                        ApplicationMessage import;
                        import.command = "import";
                        import.args.insert("url", url.toString());
                        sentMessage = m_impl->peerInstance->sendMessage(import.serialize(), timeout);
                    }
                }
            } else {
                ApplicationMessage launch;
                launch.command = "launch";
                launch.args["id"] = m_impl->instanceIdToLaunch;

                if (!m_impl->serverToJoin.isEmpty()) {
                    launch.args["server"] = m_impl->serverToJoin;
                } else if (!m_impl->worldToJoin.isEmpty()) {
                    launch.args["world"] = m_impl->worldToJoin;
                }
                if (!m_impl->profileToUse.isEmpty()) {
                    launch.args["profile"] = m_impl->profileToUse;
                }
                if (m_impl->launchOffline) {
                    launch.args["offline_enabled"] = "true";
                    launch.args["offline_name"] = m_impl->offlineName;
                }
                sentMessage = m_impl->peerInstance->sendMessage(launch.serialize(), timeout);
            }
            if (sentMessage) {
                return 0;
            }
            std::cerr << "Unable to redirect command to already running instance\n";
            return 1;
        }
    }

    // init the logger
    {
        static const QString s_baseLogFile = BuildConfig.LAUNCHER_NAME + "-%0.log";
        static const QString s_logBase = FS::PathCombine("logs", s_baseLogFile);
        if (FS::ensureFolderPathExists("logs")) {  // if this did not fail
            for (auto i = 0; i <= 4; i++) {
                if (auto oldName = s_baseLogFile.arg(i);
                    QFile::exists(oldName)) {  // do not pointlessly delete new files if the old ones are not there
                    FS::move(oldName, s_logBase.arg(i));
                }
            }
        }

        for (auto i = 4; i > 0; i--) {
            FS::move(s_logBase.arg(i - 1), s_logBase.arg(i));
        }

        m_impl->logFile = std::make_unique<QFile>(s_logBase.arg(0));
        if (!m_impl->logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            showFatalErrorMessage("The launcher data folder is not writable!",
                                  QString("The launcher couldn't create a log file - %1.\n"
                                          "\n"
                                          "Make sure you have write permissions to the data folder.\n"
                                          "(%2)\n"
                                          "\n"
                                          "The launcher cannot continue until you fix this problem.")
                                      .arg(m_impl->logFile->errorString())
                                      .arg(dataPath));
            return 1;
        }
        qInstallMessageHandler(appDebugOutput);
        qSetMessagePattern(g_defaultLogFormat);

        m_impl->logModel = std::make_unique<LogModel>(this);

        bool foundLoggingRules = false;

        auto logRulesFile = QStringLiteral("qtlogging.ini");
        auto logRulesPath = FS::PathCombine(dataPath, logRulesFile);

        qInfo() << "Testing" << logRulesPath << "...";
        foundLoggingRules = QFile::exists(logRulesPath);

        // search the dataPath()
        // seach app data standard path
        if (!foundLoggingRules && !isPortable() && dirParam.isEmpty() && dataDirEnv.isEmpty()) {
            logRulesPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, FS::PathCombine("..", logRulesFile));
            if (!logRulesPath.isEmpty()) {
                qInfo() << "Found" << logRulesPath << "...";
                foundLoggingRules = true;
            }
        }
        // seach root path
        if (!foundLoggingRules) {
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
            logRulesPath = FS::PathCombine(m_impl->rootPath, "share", BuildConfig.LAUNCHER_NAME, logRulesFile);
#else
            logRulesPath = FS::PathCombine(m_impl->rootPath, logRulesFile);
#endif
            qInfo() << "Testing" << logRulesPath << "...";
            foundLoggingRules = QFile::exists(logRulesPath);
        }

        if (foundLoggingRules) {
            // load and set logging rules
            qInfo() << "Loading logging rules from:" << logRulesPath;
            QSettings loggingRules(logRulesPath, QSettings::IniFormat);
            loggingRules.beginGroup("Rules");
            QStringList ruleNames = loggingRules.childKeys();
            QStringList rules;
            qInfo() << "Setting log rules:";
            for (const auto& ruleName : ruleNames) {
                auto rule = QString("%1=%2").arg(ruleName).arg(loggingRules.value(ruleName).toString());
                rules.append(rule);
                qInfo() << "    " << rule;
            }
            auto rulesStr = rules.join("\n");
            QLoggingCategory::setFilterRules(rulesStr);
        }

        qInfo() << "<> Log initialized.";
    }

    {
        auto migrated = handleDataMigration(
            dataPath, FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "../../PolyMC"), "PolyMC",
            "polymc.cfg");
        if (!migrated) {
            handleDataMigration(dataPath,
                                FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "../../multimc"),
                                "MultiMC", "multimc.cfg");
        }
    }

    {
        qInfo() << qPrintable(BuildConfig.LAUNCHER_DISPLAYNAME + ", " + QString(BuildConfig.LAUNCHER_COPYRIGHT).replace("\n", ", "));
        qInfo() << "Version                    :" << BuildConfig.printableVersionString();
        qInfo() << "Platform                   :" << BuildConfig.BUILD_PLATFORM;
        qInfo() << "Git commit                 :" << BuildConfig.GIT_COMMIT;
        qInfo() << "Git refspec                :" << BuildConfig.GIT_REFSPEC;
        qInfo() << "Compiled for               :" << BuildConfig.systemID();
        qInfo() << "Compiled by                :" << BuildConfig.compilerID();
        qInfo() << "Build Artifact             :" << BuildConfig.BUILD_ARTIFACT;
        qInfo() << "Updates Enabled            :" << (updaterEnabled() ? "Yes" : "No");
        if (!adjustedBy.isEmpty()) {
            qInfo() << "Work dir before adjustment :" << origcwdPath;
            qInfo() << "Work dir after adjustment  :" << QDir::currentPath();
            qInfo() << "Adjusted by                :" << adjustedBy;
        } else {
            qInfo() << "Work dir                   :" << QDir::currentPath();
        }
        qInfo() << "Binary path                :" << binPath;
        qInfo() << "Application root path      :" << m_impl->rootPath;
        if (!m_impl->instanceIdToLaunch.isEmpty()) {
            qInfo() << "ID of instance to launch   :" << m_impl->instanceIdToLaunch;
        }
        if (!m_impl->serverToJoin.isEmpty()) {
            qInfo() << "Address of server to join  :" << m_impl->serverToJoin;
        } else if (!m_impl->worldToJoin.isEmpty()) {
            qInfo() << "Name of the world to join  :" << m_impl->worldToJoin;
        }
        qInfo() << "<> Paths set.";
    }

    if (m_impl->liveCheck) {
        QFile check(g_liveCheckFile);
        if (check.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            auto payload = appID.toString().toUtf8();
            if (check.write(payload) == payload.size()) {
                check.close();
            } else {
                qWarning() << "Could not write into" << g_liveCheckFile << "error:" << check.errorString();
                check.remove();  // also closes file!
            }
        } else {
            qWarning() << "Could not open" << g_liveCheckFile << "for writing:" << check.errorString();
        }
    }

    // Initialize application settings
    {
        // Provide a fallback for migration from PolyMC
        m_impl->settings.reset(new INISettingsObject({ BuildConfig.LAUNCHER_CONFIGFILE, "polymc.cfg", "multimc.cfg" }, this));

        // Theming
        m_impl->settings->registerSetting("IconTheme", QString());
        m_impl->settings->registerSetting("ApplicationTheme", QString());
        m_impl->settings->registerSetting("BackgroundCat", QString("kitteh"));

        // Remembered state
        m_impl->settings->registerSetting("LastUsedGroupForNewInstance", QString());

        m_impl->settings->registerSetting("MenuBarInsteadOfToolBar", false);

        m_impl->settings->registerSetting("NumberOfConcurrentTasks", 10);
        m_impl->settings->registerSetting("NumberOfConcurrentDownloads", 6);
        m_impl->settings->registerSetting("NumberOfManualRetries", 1);
        m_impl->settings->registerSetting("RequestTimeout", 60);

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
        qDebug().nospace() << "Detected default console font: " << resolvedDefaultMonospace
                           << ", substitutions: " << QFont::substitutions().join(',');

        m_impl->settings->registerSetting("ConsoleFont", resolvedDefaultMonospace);
        m_impl->settings->registerSetting("ConsoleFontSize", defaultSize);
        m_impl->settings->registerSetting("ConsoleMaxLines", 100000);
        m_impl->settings->registerSetting("ConsoleOverflowStop", true);

        m_impl->logModel->setMaxLines(getConsoleMaxLines(settings()));
        m_impl->logModel->setStopOnOverflow(shouldStopOnConsoleOverflow(settings()));
        m_impl->logModel->setOverflowMessage(
            tr("Cannot display this log since the log length surpassed %1 lines.").arg(m_impl->logModel->getMaxLines()));

        // Folders
        m_impl->settings->registerSetting("InstanceDir", "instances");
        m_impl->settings->registerSetting("AdditionalInstanceDirs", QVariant(QStringList()));
        m_impl->settings->registerSetting("LastUsedInstDirForNewInstance", "");
        m_impl->settings->registerSetting({ "CentralModsDir", "ModsDir" }, "mods");
        m_impl->settings->registerSetting("IconsDir", "icons");
        m_impl->settings->registerSetting("DownloadsDir", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        m_impl->settings->registerSetting("DownloadsDirWatchRecursive", false);
        m_impl->settings->registerSetting("MoveModsFromDownloadsDir", false);
        m_impl->settings->registerSetting("SkinsDir", "skins");
        m_impl->settings->registerSetting("JavaDir", "java");

#ifdef Q_OS_MACOS
        // Folder security-scoped bookmarks
        m_impl->settings->registerSetting("InstanceDirBookmark", "");
        m_impl->settings->registerSetting("CentralModsDirBookmark", "");
        m_impl->settings->registerSetting("IconsDirBookmark", "");
        m_impl->settings->registerSetting("DownloadsDirBookmark", "");
        m_impl->settings->registerSetting("SkinsDirBookmark", "");
        m_impl->settings->registerSetting("JavaDirBookmark", "");
#endif

        // Editors
        m_impl->settings->registerSetting("JsonEditor", QString());

        // Language
        m_impl->settings->registerSetting("Language", QString());
        m_impl->settings->registerSetting("UseSystemLocale", false);

        // Console
        m_impl->settings->registerSetting("ShowConsole", false);
        m_impl->settings->registerSetting("AutoCloseConsole", false);
        m_impl->settings->registerSetting("ShowConsoleOnError", true);
        m_impl->settings->registerSetting("LogPrePostOutput", true);

        // Window Size
        m_impl->settings->registerSetting({ "LaunchMaximized", "MCWindowMaximize" }, false);
        m_impl->settings->registerSetting({ "MinecraftWinWidth", "MCWindowWidth" }, 854);
        m_impl->settings->registerSetting({ "MinecraftWinHeight", "MCWindowHeight" }, 480);

        // Proxy Settings
        m_impl->settings->registerSetting("ProxyType", "None");
        m_impl->settings->registerSetting({ "ProxyAddr", "ProxyHostName" }, "127.0.0.1");
        m_impl->settings->registerSetting("ProxyPort", 8080);
        m_impl->settings->registerSetting({ "ProxyUser", "ProxyUsername" }, "");
        m_impl->settings->registerSetting({ "ProxyPass", "ProxyPassword" }, "");

        // Memory
        m_impl->settings->registerSetting({ "MinMemAlloc", "MinMemoryAlloc" }, 512);
        m_impl->settings->registerSetting({ "MaxMemAlloc", "MaxMemoryAlloc" }, SysInfo::defaultMaxJvmMem());
        m_impl->settings->registerSetting("PermGen", 128);
        m_impl->settings->registerSetting("LowMemWarning", true);

        // Java Settings
        m_impl->settings->registerSetting("JavaPath", "");
        m_impl->settings->registerSetting("JavaSignature", "");
        m_impl->settings->registerSetting("JavaArchitecture", "");
        m_impl->settings->registerSetting("JavaRealArchitecture", "");
        m_impl->settings->registerSetting("JavaVersion", "");
        m_impl->settings->registerSetting("JavaVendor", "");
        m_impl->settings->registerSetting("LastHostname", "");
        m_impl->settings->registerSetting("PinnedJavaPaths", "{}");
        m_impl->settings->registerSetting("JvmArgs", "");
        m_impl->settings->registerSetting("IgnoreJavaCompatibility", false);
        m_impl->settings->registerSetting("IgnoreJavaWizard", false);
        auto defaultEnableAutoJava = m_impl->settings->get("JavaPath").toString().isEmpty();
        m_impl->settings->registerSetting("AutomaticJavaSwitch", defaultEnableAutoJava);
        m_impl->settings->registerSetting("AutomaticJavaDownload", defaultEnableAutoJava);
        m_impl->settings->registerSetting("UserAskedAboutAutomaticJavaDownload", false);

        // Legacy settings
        m_impl->settings->registerSetting("OnlineFixes", false);

        // Native library workarounds
        m_impl->settings->registerSetting("UseNativeOpenAL", false);
        m_impl->settings->registerSetting("CustomOpenALPath", "");
        m_impl->settings->registerSetting("UseNativeGLFW", false);
        m_impl->settings->registerSetting("CustomGLFWPath", "");
        m_impl->settings->registerSetting("UseNativeSDL", false);
        m_impl->settings->registerSetting("CustomSDLPath", "");

        // Performance related options
        m_impl->settings->registerSetting("EnableFeralGamemode", false);
        m_impl->settings->registerSetting("EnableMangoHud", false);
        m_impl->settings->registerSetting("UseDiscreteGpu", false);
        m_impl->settings->registerSetting("UseZink", false);

        // Game time
        m_impl->settings->registerSetting("ShowGameTime", true);
        m_impl->settings->registerSetting("ShowGlobalGameTime", true);
        m_impl->settings->registerSetting("RecordGameTime", true);
        m_impl->settings->registerSetting("ShowGameTimeWithoutDays", false);

        // Minecraft mods
        m_impl->settings->registerSetting("ModMetadataDisabled", false);
        m_impl->settings->registerSetting("ModDependenciesDisabled", false);
        m_impl->settings->registerSetting("SkipModpackUpdatePrompt", false);
        m_impl->settings->registerSetting("ShowModIncompat", false);
        m_impl->settings->registerSetting("DownloadGameFilesDuringInstanceCreation", true);
        m_impl->settings->registerSetting("ModUpdateReleaseTypes", "[]");

        // Minecraft offline player name
        m_impl->settings->registerSetting("LastOfflinePlayerName", "");

        // Wrapper command for launch
        m_impl->settings->registerSetting("WrapperCommand", "");

        // Custom Commands
        m_impl->settings->registerSetting({ "PreLoadCommand", "PreLoadCmd" }, "");
        m_impl->settings->registerSetting({ "PreLaunchCommand", "PreLaunchCmd" }, "");
        m_impl->settings->registerSetting({ "PostExitCommand", "PostExitCmd" }, "");

        // The cat
        m_impl->settings->registerSetting("EnableCat", true);
        m_impl->settings->registerSetting("TheCat", false);
        m_impl->settings->registerSetting("CatOpacity", 100);
        m_impl->settings->registerSetting("CatFit", "fit");

        m_impl->settings->registerSetting("StatusBarVisible", true);

        m_impl->settings->registerSetting("ToolbarsLocked", false);

        // Instance
        m_impl->settings->registerSetting("InstSortMode", "Name");
        m_impl->settings->registerSetting("InstRenamingMode", "AskEverytime");
        m_impl->settings->registerSetting("EditInstanceOnDoubleClick", false);
        m_impl->settings->registerSetting("SelectedInstance", QString());

        // Window state and geometry
        m_impl->settings->registerSetting("MainWindowState", "");
        m_impl->settings->registerSetting("MainWindowGeometry", "");

        m_impl->settings->registerSetting("ConsoleWindowState", "");
        m_impl->settings->registerSetting("ConsoleWindowGeometry", "");

        m_impl->settings->registerSetting("SettingsGeometry", "");

        m_impl->settings->registerSetting("PagedGeometry", "");

        m_impl->settings->registerSetting("NewInstanceGeometry", "");

        m_impl->settings->registerSetting("UpdateDialogGeometry", "");

        m_impl->settings->registerSetting("NewsGeometry", "");

        m_impl->settings->registerSetting("ModDownloadGeometry", "");
        m_impl->settings->registerSetting("RPDownloadGeometry", "");
        m_impl->settings->registerSetting("TPDownloadGeometry", "");
        m_impl->settings->registerSetting("ShaderDownloadGeometry", "");
        m_impl->settings->registerSetting("DataPackDownloadGeometry", "");

        // data pack window
        // in future, more pages may be added - so this name is chosen to avoid needing migration
        m_impl->settings->registerSetting("WorldManagementGeometry", "");

        // HACK: This code feels so stupid is there a less stupid way of doing this?
        {
            m_impl->settings->registerSetting("PastebinURL", "");
            m_impl->settings->registerSetting("PastebinType", static_cast<int>(PasteUpload::Type::Mclogs));
            m_impl->settings->registerSetting("PastebinCustomAPIBase", "");

            QString pastebinURL = m_impl->settings->get("PastebinURL").toString();

            bool userHadDefaultPastebin = pastebinURL == "https://0x0.st";
            if (!pastebinURL.isEmpty() && !userHadDefaultPastebin) {
                m_impl->settings->set("PastebinType", static_cast<int>(PasteUpload::Type::NullPointer));
                m_impl->settings->set("PastebinCustomAPIBase", pastebinURL);
                m_impl->settings->reset("PastebinURL");
            }

            bool ok = false;
            int pasteType = m_impl->settings->get("PastebinType").toInt(&ok);
            // If PastebinType is invalid then reset the related settings.
            if (!ok || !PasteUpload::Type(pasteType).isValid()) {
                m_impl->settings->reset("PastebinType");
                m_impl->settings->reset("PastebinCustomAPIBase");
            }
        }
        {
            auto resetIfInvalid = [this](const Setting* setting) {
                if (const auto value = setting->get().toString(); !value.isEmpty()) {
                    if (const QUrl url(value); !url.isValid() || (url.scheme() != "http" && url.scheme() != "https")) {
                        m_impl->settings->reset(setting->id());
                    }
                }
            };

            // Meta URL
            resetIfInvalid(m_impl->settings->registerSetting("MetaURLOverride", "").get());

            // Resource URL
            resetIfInvalid(m_impl->settings->registerSetting({ "ResourceURLOverride", "ResourceURL" }, "").get());

            // Legacy FML libs URL
            resetIfInvalid(m_impl->settings->registerSetting("LegacyFMLLibsURLOverride", "").get());
        }

        m_impl->settings->registerSetting("MetaRefreshOnLaunch", true);
        m_impl->settings->registerSetting("CloseAfterLaunch", false);
        m_impl->settings->registerSetting("QuitAfterGameStop", false);

        m_impl->settings->registerSetting("Env", "{}");

        m_impl->settings->registerSetting("WorldTools", "{}");

        // Custom Microsoft Authentication Client ID
        m_impl->settings->registerSetting("MSAClientIDOverride", "");

        // Custom Flame API Key
        m_impl->settings->registerSetting({ "FlameKeyOverride", "CFKeyOverride" }, "");

        m_impl->settings->registerSetting("FallbackMRBlockedMods", true);
        m_impl->settings->registerSetting("ModrinthToken", "");
        m_impl->settings->registerSetting("UserAgentOverride", "");

        // FTBApp instances
        m_impl->settings->registerSetting("FTBAppInstancesPath", "");

        // Custom Technic Client ID
        m_impl->settings->registerSetting("TechnicClientID", "");

        // Init page provider
        {
            m_impl->globalSettingsProvider = std::make_unique<GenericPageProvider>(tr("Settings"));
            m_impl->globalSettingsProvider->addPage<LauncherPage>();
            m_impl->globalSettingsProvider->addPage<LanguagePage>();
            m_impl->globalSettingsProvider->addPage<AppearancePage>();
            m_impl->globalSettingsProvider->addPage<MinecraftPage>();
            m_impl->globalSettingsProvider->addPage<JavaPage>();
            m_impl->globalSettingsProvider->addPage<AccountListPage>();
            m_impl->globalSettingsProvider->addPage<APIPage>();
            m_impl->globalSettingsProvider->addPage<ExternalToolsPage>();
            m_impl->globalSettingsProvider->addPage<ProxyPage>();
        }

        PixmapCache::setInstance(new PixmapCache(this));

        qInfo() << "<> Settings loaded.";
    }

    // Initialize playtime settings, stored separately so this data can be synced
    // independently of machine-specific configuration
    {
        m_impl->playtimeSettings.reset(new INISettingsObject(QString("playtime.cfg"), this));
        m_impl->playtimeSettings->registerSetting("TotalPlayTime", 0);
        m_impl->playtimeSettings->registerSetting("TotalPlayTimeMigrated", false);
    }

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installFactory(groupViewAccessibleFactory);
#endif /* !QT_NO_ACCESSIBILITY */

    // initialize network access and proxy setup
    {
        m_impl->network.reset(new QNetworkAccessManager());
        QString proxyTypeStr = settings()->get("ProxyType").toString();
        QString addr = settings()->get("ProxyAddr").toString();
        int port = settings()->get("ProxyPort").value<qint16>();
        QString user = settings()->get("ProxyUser").toString();
        QString pass = settings()->get("ProxyPass").toString();
        updateProxySettings(proxyTypeStr, addr, port, user, pass);
        qInfo() << "<> Network done.";
    }

    // Instance icons
    {
        auto setting = APPLICATION->settings()->getSetting("IconsDir");
        QStringList instFolders = { ":/icons/multimc/32x32/instances/", ":/icons/multimc/50x50/instances/",
                                    ":/icons/multimc/128x128/instances/", ":/icons/multimc/scalable/instances/" };
        m_impl->icons.reset(new IconList(instFolders, setting->get().toString()));
        connect(setting.get(), &Setting::SettingChanged, this,
                [this](const Setting&, const QVariant& value) { m_impl->icons->directoryChanged(value.toString()); });
        qInfo() << "<> Instance icons initialized.";
    }

    // Themes
    m_impl->themeManager = std::make_unique<ThemeManager>();

#ifdef Q_OS_MACOS
    // for macOS: getting directory settings will generate URL security-scoped bookmarks if needed and not present
    // this facilitates a smooth transition from a non-sandboxed version of the launcher, that likely can access the directory,
    // and a sandboxed version that can't access the directory without a bookmark
    // this section can likely be removed once the sandboxed version has been released for a while and migrations aren't done anymore
    {
        m_impl->settings->get("InstanceDir");
        m_impl->settings->get("CentralModsDir");
        m_impl->settings->get("IconsDir");
        m_impl->settings->get("DownloadsDir");
        m_impl->settings->get("SkinsDir");
        m_impl->settings->get("JavaDir");
    }
#endif

    // initialize and load all instances
    {
        auto instDirSetting = m_impl->settings->getSetting("InstanceDir");
        auto additionalInstanceDirsSetting = m_impl->settings->getSetting("AdditionalInstanceDirs");
        // instance path: check for problems with '!' in instance path and warn the user in the log
        // and remember that we have to show him a dialog when the gui starts (if it does so)
        QString instDir = m_impl->settings->get("InstanceDir").toString();
        qInfo() << "Instance path              :" << instDir;
        if (FS::checkProblemticPathJava(QDir(instDir))) {
            qWarning() << "Your instance path contains \'!\' and this is known to cause java problems!";
        }
        QStringList additionalDirs = m_impl->settings->get("AdditionalInstanceDirs").toStringList();
        QStringList allInstDirs;
        allInstDirs << instDir;
        for (const auto& dir : additionalDirs) {
            if (!dir.isEmpty() && !allInstDirs.contains(dir)) {
                allInstDirs << dir;
            }
        }

        m_impl->instances.reset(new InstanceList(m_impl->settings.get(), allInstDirs, this));
        connect(instDirSetting.get(), &Setting::SettingChanged, m_impl->instances.get(), &InstanceList::on_InstFolderChanged);
        connect(additionalInstanceDirsSetting.get(), &Setting::SettingChanged, m_impl->instances.get(),
                &InstanceList::on_InstFolderChanged);
        qInfo() << "Loading Instances...";
        m_impl->instances->loadList();
        qInfo() << "<> Instances loaded.";
    }

    // and accounts
    {
        m_impl->accounts.reset(new AccountList(this));
        qInfo() << "Loading accounts...";
        m_impl->accounts->setListFilePath("accounts.json", true);
        m_impl->accounts->loadList();
        m_impl->accounts->fillQueue();
        qInfo() << "<> Accounts loaded.";
    }

    // init the http meta cache
    {
        m_impl->metacache.reset(new HttpMetaCache("metacache"));
        m_impl->metacache->addBase("asset_indexes", QDir("assets/indexes").absolutePath());
        m_impl->metacache->addBase("libraries", QDir("libraries").absolutePath());
        m_impl->metacache->addBase("fmllibs", QDir("mods/minecraftforge/libs").absolutePath());
        m_impl->metacache->addBase("general", QDir("cache").absolutePath());
        m_impl->metacache->addBase("ATLauncherPacks", QDir("cache/ATLauncherPacks").absolutePath());
        m_impl->metacache->addBase("FTBPacks", QDir("cache/FTBPacks").absolutePath());
        m_impl->metacache->addBase("TechnicPacks", QDir("cache/TechnicPacks").absolutePath());
        m_impl->metacache->addBase("FlamePacks", QDir("cache/FlamePacks").absolutePath());
        m_impl->metacache->addBase("FlameMods", QDir("cache/FlameMods").absolutePath());
        m_impl->metacache->addBase("ModrinthPacks", QDir("cache/ModrinthPacks").absolutePath());
        m_impl->metacache->addBase("ModrinthModpacks", QDir("cache/ModrinthModpacks").absolutePath());
        m_impl->metacache->addBase("translations", QDir("translations").absolutePath());
        m_impl->metacache->addBase("meta", QDir("meta").absolutePath());
        m_impl->metacache->addBase("java", QDir("cache/java").absolutePath());
        m_impl->metacache->addBase("feed", QDir("cache/feed").absolutePath());
        m_impl->metacache->Load();
        qInfo() << "<> Cache initialized.";
    }

    // load translations
    {
        m_impl->translations.reset(new TranslationsModel("translations"));
        m_impl->translations->downloadIndex();
        qInfo() << "Your language is" << m_impl->translations->selectedLanguage();
        qInfo() << "<> Translations loaded.";
    }

    // FIXME: what to do with these?
    m_impl->profilers.insert("jprofiler", std::shared_ptr<BaseProfilerFactory>(new JProfilerFactory()));
    m_impl->profilers.insert("jvisualvm", std::shared_ptr<BaseProfilerFactory>(new JVisualVMFactory()));
    m_impl->profilers.insert("generic", std::shared_ptr<BaseProfilerFactory>(new GenericProfilerFactory()));
    for (const auto& profiler : m_impl->profilers.values()) {
        profiler->registerSettings(m_impl->settings.get());
    }

#ifdef Q_OS_MACOS
    connect(this, &Application::clickedOnDock, this, [this]() { this->showMainWindow(); });
#endif

    connect(&app, &QApplication::aboutToQuit, this, [this]() {
        if (m_impl->instances) {
            // save any remaining instance state
            m_impl->instances->saveNow();
        }
        if (m_impl->logFile) {
            m_impl->logFile->flush();
            m_impl->logFile->close();
        }
    });

    updateCapabilities();

    detectLibraries();

    // check update locks
    {
        auto updateLogPath = FS::PathCombine(m_impl->dataPath, "logs", "prism_launcher_update.log");

        auto updateLock = QFileInfo(FS::PathCombine(m_impl->dataPath, "prism_launcher_update.lock"));
        if (updateLock.exists()) {
            auto [timestamp, from, to, target, lockDataPath] = readLockFile(updateLock.absoluteFilePath());
            auto infoMsg = tr("This installation has a update lock file present at: %1\n"
                              "\n"
                              "Timestamp: %2\n"
                              "Updating from version %3 to %4\n"
                              "Target install path: %5\n"
                              "Data Path: %6"
                              "\n"
                              "This likely means that a update attempt failed. Please ensure your installation is in working order before "
                              "proceeding.\n"
                              "Check the Prism Launcher updater log at: \n"
                              "%7\n"
                              "for details on the last update attempt.\n"
                              "\n"
                              "To delete this lock and proceed select \"Ignore\" below.")
                               .arg(updateLock.absoluteFilePath())
                               .arg(timestamp.toString(Qt::ISODate), from, to, target, lockDataPath)
                               .arg(updateLogPath);
            auto msgBox = QMessageBox(QMessageBox::Warning, tr("Update In Progress"), infoMsg, QMessageBox::Ignore | QMessageBox::Abort);
            msgBox.setDefaultButton(QMessageBox::Abort);
            msgBox.setModal(true);
            auto maybeRes = FS::read(updateLogPath);
            if (!maybeRes) {
                qFatal("Failed to read update log: %s", maybeRes.error().toUtf8().constData());
            }
            msgBox.setDetailedText(maybeRes.value());
            msgBox.setMinimumWidth(460);
            msgBox.adjustSize();
            auto res = msgBox.exec();
            switch (res) {
                case QMessageBox::Ignore: {
                    FS::deletePath(updateLock.absoluteFilePath());
                    break;
                }
                case QMessageBox::Abort:
                    [[fallthrough]];
                default: {
                    qDebug() << "Exiting because update lockfile is present";
                    return 1;
                }
            }
        }

        auto updateFailMarker = QFileInfo(FS::PathCombine(m_impl->dataPath, ".prism_impl->launcher_update.fail"));
        if (updateFailMarker.exists()) {
            auto infoMsg = tr("An update attempt failed\n"
                              "\n"
                              "Please ensure your installation is in working order before "
                              "proceeding.\n"
                              "Check the Prism Launcher updater log at: \n"
                              "%1\n"
                              "for details on the last update attempt.")
                               .arg(updateLogPath);
            auto msgBox = QMessageBox(QMessageBox::Warning, tr("Update Failed"), infoMsg, QMessageBox::Ignore | QMessageBox::Abort);
            msgBox.setDefaultButton(QMessageBox::Abort);
            msgBox.setModal(true);
            auto maybeRes = FS::read(updateLogPath);
            if (!maybeRes) {
                qFatal("Failed to read update log: %s", maybeRes.error().toUtf8().constData());
            }
            msgBox.setDetailedText(maybeRes.value());
            msgBox.setMinimumWidth(460);
            msgBox.adjustSize();
            auto res = msgBox.exec();
            switch (res) {
                case QMessageBox::Ignore: {
                    FS::deletePath(updateFailMarker.absoluteFilePath());
                    break;
                }
                case QMessageBox::Abort:
                    [[fallthrough]];
                default: {
                    qDebug() << "Exiting because update lockfile is present";
                    return 1;
                }
            }
        }

        auto updateSuccessMarker = QFileInfo(FS::PathCombine(m_impl->dataPath, ".prism_impl->launcher_update.success"));
        if (updateSuccessMarker.exists()) {
            auto infoMsg = tr("Update succeeded\n"
                              "\n"
                              "You are now running %1 .\n"
                              "Check the Prism Launcher updater log at: \n"
                              "%2\n"
                              "for details.")
                               .arg(BuildConfig.printableVersionString())
                               .arg(updateLogPath);
            auto* msgBox = new QMessageBox(QMessageBox::Information, tr("Update Succeeded"), infoMsg, QMessageBox::Ok);
            msgBox->setDefaultButton(QMessageBox::Ok);
            auto res = FS::read(updateLogPath);
            if (!res) {
                qFatal("Failed to read update log: %s", res.error().toUtf8().constData());
            }
            msgBox->setDetailedText(res.value());
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->setMinimumWidth(460);
            msgBox->adjustSize();
            msgBox->open();
            FS::deletePath(updateSuccessMarker.absoluteFilePath());
        }
    }

    // notify user if /tmp is mounted with `noexec` (#1693)
    QString jvmArgs = m_impl->settings->get("JvmArgs").toString();
    if (jvmArgs.indexOf("java.io.tmpdir") == -1) { /* java.io.tmpdir is a valid workaround, so don't annoy */
        bool isTmpNoexec = false;

#if defined(Q_OS_LINUX)

        struct statvfs tmpStat{};
        statvfs("/tmp", &tmpStat);
        isTmpNoexec = ((tmpStat.f_flag & ST_NOEXEC) != 0U);

#elif defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)

        struct statfs tmp_stat;
        statfs("/tmp", &tmp_stat);
        isTmpNoexec = ((tmp_stat.f_flags & MNT_NOEXEC) != 0U);

#endif

        if (isTmpNoexec) {
            auto infoMsg =
                tr("Your /tmp directory is currently mounted with the 'noexec' flag enabled.\n"
                   "Some versions of Minecraft may not launch.\n"
                   "\n"
                   "You may solve this issue by remounting /tmp as 'exec' or setting "
                   "the java.io.tmpdir JVM argument to a writeable directory in a "
                   "filesystem where the 'exec' flag is set (e.g., /home/user/.local/tmp)\n");
            auto* msgBox = new QMessageBox(QMessageBox::Information, tr("Incompatible system configuration"), infoMsg, QMessageBox::Ok);
            msgBox->setDefaultButton(QMessageBox::Ok);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->setMinimumWidth(460);
            msgBox->adjustSize();
            msgBox->open();
        }
    }

    if (qgetenv("XDG_CURRENT_DESKTOP") == "gamescope") {
        installEventFilter(new ToolTipFilter);
    }

    // the setup wizard applies the selected theme itself before it is shown
    if (!createSetupWizard()) {
        m_impl->themeManager->applyCurrentlySelectedTheme(true);
    }
    performMainStartupAction();

    return QApplication::exec();
}

bool Application::createSetupWizard()
{
    bool javaRequired = [this]() {
        if (BuildConfig.JAVA_DOWNLOADER_ENABLED && settings()->get("AutomaticJavaDownload").toBool()) {
            return false;
        }
        bool ignoreJavaWizard = settings()->get("IgnoreJavaWizard").toBool();
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
        return actualPath.isNull();
    }();
    bool askjava = BuildConfig.JAVA_DOWNLOADER_ENABLED && !javaRequired && !settings()->get("AutomaticJavaDownload").toBool() &&
                   !settings()->get("AutomaticJavaSwitch").toBool() && !settings()->get("UserAskedAboutAutomaticJavaDownload").toBool();
    bool languageRequired = settings()->get("Language").toString().isEmpty();
    bool pasteInterventionRequired = settings()->get("PastebinURL") != "";
    bool validWidgets = m_impl->themeManager->isValidApplicationTheme(settings()->get("ApplicationTheme").toString());
    bool validIcons = m_impl->themeManager->isValidIconTheme(settings()->get("IconTheme").toString());
    bool login = !m_impl->accounts->anyAccountIsValid() && (capabilities().testAnyFlags(Application::SupportsMSA));
    bool themeInterventionRequired = !validWidgets || !validIcons;
    bool wizardRequired = javaRequired || languageRequired || pasteInterventionRequired || themeInterventionRequired || askjava || login;
    if (wizardRequired) {
        const bool darkMode = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
        // set default theme after going into theme wizard
        if (!validIcons) {
            QString defaultIconTheme;
            if (QApplication::style()->name() == "breeze") {
                defaultIconTheme = darkMode ? "breeze_dark" : "breeze_light";
            } else {
                defaultIconTheme = darkMode ? "flat_white" : "flat";
            }

            settings()->set("IconTheme", defaultIconTheme);
        }
        if (!validWidgets) {
#if defined(Q_OS_LINUX)
            const QString style = "system";
#else
            const QString style = darkMode ? QStringLiteral("dark") : QStringLiteral("bright");

#endif

            settings()->set("ApplicationTheme", style);
        }

        m_impl->themeManager->applyCurrentlySelectedTheme(true);

        SetupWizard setupWizard;
        if (languageRequired) {
            setupWizard.addPage(new LanguageWizardPage(&setupWizard));
        }

        if (javaRequired) {
            setupWizard.addPage(new JavaWizardPage(&setupWizard));
        } else if (askjava) {
            setupWizard.addPage(new AutoJavaWizardPage(&setupWizard));
        }

        if (pasteInterventionRequired) {
            setupWizard.addPage(new PasteWizardPage(&setupWizard));
        }

        if (themeInterventionRequired) {
            setupWizard.addPage(new ThemeWizardPage(&setupWizard));
        }

        if (login) {
            setupWizard.addPage(new LoginWizardPage(&setupWizard));
        }
        if (setupWizard.exec() != QDialog::Accepted) {
            qWarning() << "Setup wizard was not completed; continuing with the current settings";
        }
    }

    return wizardRequired;
}

bool Application::updaterEnabled()
{
#if defined(Q_OS_MAC)
    return BuildConfig.UPDATER_ENABLED;
#else
    return BuildConfig.UPDATER_ENABLED && QFileInfo(FS::PathCombine(m_impl->rootPath, updaterBinaryName())).isFile();
#endif
}

QString Application::updaterBinaryName()
{
    auto exeName = QStringLiteral("%1_updater").arg(BuildConfig.LAUNCHER_APP_BINARY_NAME);
#if defined Q_OS_WIN32
    exeName.append(".exe");
#else
    exeName.prepend("bin/");
#endif
    return exeName;
}

bool Application::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != QApplication::instance()) {
        return false;
    }

#ifdef Q_OS_MACOS
    if (event->type() == QEvent::ApplicationStateChange) {
        auto ev = static_cast<QApplicationStateChangeEvent*>(event);

        if (m_impl->prevAppState == Qt::ApplicationActive && ev->applicationState() == Qt::ApplicationActive) {
            emit clickedOnDock();
        }
        m_impl->prevAppState = ev->applicationState();
    }
#endif

    if (event->type() == QEvent::FileOpen) {
        if (!m_impl->mainWindow) {
            showMainWindow(false);
        }
        auto* ev = static_cast<QFileOpenEvent*>(event);
        m_impl->mainWindow->processURLs({ ev->url() });
    }

    return QObject::eventFilter(watched, event);
}

SettingsObject* Application::settings() const
{
    return m_impl->settings.get();
}

SettingsObject* Application::playtimeSettings() const
{
    return m_impl->playtimeSettings.get();
}

void Application::performMainStartupAction()
{
    m_impl->initialized = true;

    if (!m_impl->instanceIdToLaunch.isEmpty()) {
        auto* inst = instances()->getInstanceById(m_impl->instanceIdToLaunch);
        if (inst) {
            MinecraftTarget::Ptr targetToJoin = nullptr;
            MinecraftAccountPtr accountToUse = nullptr;

            qDebug() << "<> Instance" << m_impl->instanceIdToLaunch << "launching";
            if (!m_impl->serverToJoin.isEmpty()) {
                // FIXME: validate the server string
                targetToJoin.reset(new MinecraftTarget(MinecraftTarget::parse(m_impl->serverToJoin, false)));
                qDebug() << "   Launching with server" << m_impl->serverToJoin;
            } else if (!m_impl->worldToJoin.isEmpty()) {
                targetToJoin.reset(new MinecraftTarget(MinecraftTarget::parse(m_impl->worldToJoin, true)));
                qDebug() << "   Launching with world" << m_impl->worldToJoin;
            }

            if (!m_impl->profileToUse.isEmpty()) {
                accountToUse = accounts()->getAccountByProfileName(m_impl->profileToUse);
                if (!accountToUse) {
                    return;
                }
                qDebug() << "   Launching with account" << m_impl->profileToUse;
            }

            launch(inst, m_impl->launchOffline ? LaunchMode::Offline : LaunchMode::Normal, targetToJoin, accountToUse, m_impl->offlineName);

            if (!m_impl->showMainWindow) {
                return;
            }
        }
    }
    if (!m_impl->instanceIdToShowWindowOf.isEmpty()) {
        auto* inst = instances()->getInstanceById(m_impl->instanceIdToShowWindowOf);
        if (inst) {
            qDebug() << "<> Showing window of instance " << m_impl->instanceIdToShowWindowOf;
            showInstanceWindow(inst);
            return;
        }
    }
    if (!m_impl->mainWindow) {
        // normal main window
        showMainWindow(false);
        qDebug() << "<> Main window shown.";
    }

    // initialize the updater
    if (updaterEnabled()) {
        qDebug() << "Initializing updater";
#ifdef Q_OS_MAC
#if defined(SPARKLE_ENABLED)
        m_impl->updater.reset(new MacSparkleUpdater());
#endif
#else
        m_impl->updater.reset(new PrismExternalUpdater(m_impl->mainWindow, m_impl->rootPath, m_impl->dataPath));
#endif
        qDebug() << "<> Updater started.";
    }

    {  // delete instances tmp dirctory
        auto instDir = m_impl->settings->get("InstanceDir").toString();
        const QString tempRoot = FS::PathCombine(instDir, ".tmp");
        FS::deletePath(tempRoot);
    }

    if (!m_impl->urlsToImport.isEmpty()) {
        qDebug() << "<> Importing from url:" << m_impl->urlsToImport;
        m_impl->mainWindow->processURLs(m_impl->urlsToImport);
    }
}

void Application::messageReceived(const QByteArray& message)
{
    ApplicationMessage received;
    auto res = received.parse(message);
    if (!res) {
        qWarning() << "Received invalid message:" << res.error();
        return;
    }

    auto& command = received.command;

    if (!m_impl->initialized) {
        bool isLoginAtempt = false;
        if (command == "import") {
            QString url = received.args["url"];
            isLoginAtempt = !url.isEmpty() && normalizeImportUrl(url).scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME;
        }
        if (!isLoginAtempt) {
            qDebug() << "Received message" << message << "while still initializing. It will be ignored.";
            return;
        }
    }

    if (command == "activate") {
        showMainWindow();
    } else if (command == "import") {
        QString url = received.args["url"];
        if (url.isEmpty()) {
            qWarning() << "Received" << command << "message without a zip path/URL.";
            return;
        }
        if (!m_impl->mainWindow) {
            showMainWindow(false);
        }
        m_impl->mainWindow->processURLs({ normalizeImportUrl(url) });
    } else if (command == "launch") {
        QString id = received.args["id"];
        QString server = received.args["server"];
        QString world = received.args["world"];
        QString profile = received.args["profile"];
        bool offline = received.args["offline_enabled"] == "true";
        QString offlineName = received.args["offline_name"];

        MinecraftInstance* instance = nullptr;
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

        MinecraftTarget::Ptr serverObject = nullptr;
        if (!server.isEmpty()) {
            serverObject = std::make_shared<MinecraftTarget>(MinecraftTarget::parse(server, false));
        } else if (!world.isEmpty()) {
            serverObject = std::make_shared<MinecraftTarget>(MinecraftTarget::parse(world, true));
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

        launch(instance, offline ? LaunchMode::Offline : LaunchMode::Normal, serverObject, accountObject, offlineName);
    } else {
        qWarning() << "Received invalid message" << message;
    }
}

TranslationsModel* Application::translations()
{
    return m_impl->translations.get();
}

JavaInstallList* Application::javalist()
{
    if (!m_impl->javalist) {
        m_impl->javalist.reset(new JavaInstallList());
    }
    return m_impl->javalist.get();
}

QIcon Application::logo()
{
    return QIcon(":/" + BuildConfig.LAUNCHER_SVGFILENAME);
}

InstanceList* Application::instances() const
{
    return m_impl->instances.get();
}

IconList* Application::icons() const
{
    return m_impl->icons.get();
}

AccountList* Application::accounts() const
{
    return m_impl->accounts.get();
}

const QMap<QString, std::shared_ptr<BaseProfilerFactory>>& Application::profilers() const
{
    return m_impl->profilers;
}

ThemeManager* Application::themeManager()
{
    return m_impl->themeManager.get();
}

ExternalUpdater* Application::updater()
{
    return m_impl->updater.get();
}

bool Application::openJsonEditor(const QString& filename)
{
    const QString file = QDir::current().absoluteFilePath(filename);
    if (m_impl->settings->get("JsonEditor").toString().isEmpty()) {
        return DesktopServices::openUrl(QUrl::fromLocalFile(file));
    }
    // return DesktopServices::openFile(m_impl->settings->get("JsonEditor").toString(), file);
    return DesktopServices::run(m_impl->settings->get("JsonEditor").toString(), { file });
}

bool Application::launch(MinecraftInstance* instance,
                         LaunchMode mode,
                         MinecraftTarget::Ptr targetToJoin,
                         MinecraftAccountPtr accountToUse,
                         const QString& offlineName)
{
    if (m_impl->updateRunning) {
        qDebug() << "Cannot launch instances while an update is running. Please try again when updates are completed.";
    } else if (instance->canLaunch()) {
        if (instance->window() != nullptr) {
            if (!instance->window()->saveAll()) {
                return false;
            }
        }
        instance->setLaunchController(std::make_unique<LaunchController>());
        auto* controller = instance->launchController();
        controller->setInstance(instance);
        controller->setLaunchMode(mode);
        controller->setProfiler(profilers().value(instance->settings()->get("Profiler").toString(), nullptr).get());
        controller->setTargetToJoin(std::move(targetToJoin));
        controller->setAccountToUse(std::move(accountToUse));
        controller->setOfflineName(offlineName);
        if (instance->window() != nullptr) {
            controller->setParentWidget(instance->window());
        } else if (m_impl->mainWindow != nullptr) {
            controller->setParentWidget(m_impl->mainWindow);
        }
        connect(controller, &LaunchController::finished, this, &Application::controllerFinished);
        addRunningInstance();
        QMetaObject::invokeMethod(controller, &Task::start, Qt::QueuedConnection);
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

bool Application::kill(BaseInstance* instance)
{
    if (!instance->isRunning()) {
        qWarning() << "Attempted to kill instance" << instance->id() << ", which isn't running.";
        return false;
    }
    if (instance->launchController() != nullptr) {
        return instance->launchController()->abort();
    }
    return true;
}

void Application::addRunningInstance()
{
    m_impl->runningInstances++;
    if (m_impl->runningInstances == 1) {
        emit updateAllowedChanged(false);
    }
}

void Application::subRunningInstance()
{
    if (m_impl->runningInstances == 0) {
        qCritical() << "Something went really wrong and we now have less than 0 running instances... WTF";
        return;
    }
    m_impl->runningInstances--;
    if (m_impl->runningInstances == 0) {
        emit updateAllowedChanged(true);
    }
}

bool Application::shouldExitNow() const
{
    return m_impl->runningInstances == 0 && m_impl->openWindows == 0;
}

bool Application::updatesAreAllowed() const
{
    return m_impl->runningInstances == 0;
}

void Application::updateIsRunning(bool running)
{
    m_impl->updateRunning = running;
}

void Application::controllerFinished()
{
    auto* controller = qobject_cast<LaunchController*>(sender());
    if (!controller) {
        return;
    }

    auto* instance = controller->instance();

    const bool wasSuccessful = controller->wasSuccessful();
    // on success, do...
    if (wasSuccessful && instance->settings()->get("AutoCloseConsole").toBool()) {
        if (instance->window() != nullptr) {
            QMetaObject::invokeMethod(instance->window(), &QWidget::close, Qt::QueuedConnection);
        }
    }
    instance->setLaunchController(nullptr);
    subRunningInstance();

    // quit when there are no more windows.
    if (shouldExitNow()) {
        QApplication::exit(wasSuccessful ? 0 : 1);
    }
}

void Application::ShowGlobalSettings(class QWidget* parent, QString openPage)
{
    if (!m_impl->globalSettingsProvider) {
        return;
    }
    emit globalSettingsAboutToOpen();
    {
        SettingsObject::Lock lock(APPLICATION->settings());
        PageDialog dlg(m_impl->globalSettingsProvider.get(), std::move(openPage), parent);
        connect(&dlg, &PageDialog::applied, this, &Application::globalSettingsApplied);
        dlg.exec();
    }
}

MainWindow* Application::showMainWindow(bool minimized)
{
    if (m_impl->mainWindow) {
        m_impl->mainWindow->setWindowState(m_impl->mainWindow->windowState() & ~Qt::WindowMinimized);
        m_impl->mainWindow->raise();
        m_impl->mainWindow->activateWindow();
    } else {
        m_impl->mainWindow = new MainWindow();
        m_impl->mainWindow->restoreState(QByteArray::fromBase64(APPLICATION->settings()->get("MainWindowState").toString().toUtf8()));
        m_impl->mainWindow->restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("MainWindowGeometry").toString().toUtf8()));

        if (minimized) {
            m_impl->mainWindow->showMinimized();
        } else {
            m_impl->mainWindow->show();
        }

        m_impl->mainWindow->checkInstancePathForProblems();
        connect(this, &Application::updateAllowedChanged, m_impl->mainWindow, &MainWindow::updatesAllowedChanged);
        connect(m_impl->mainWindow, &MainWindow::isClosing, this, [this] {
            m_impl->mainWindow = nullptr;
            on_windowClose();
        });
        m_impl->openWindows++;
    }
    return m_impl->mainWindow;
}

ViewLogWindow* Application::showLogWindow()
{
    if (m_impl->viewLogWindow) {
        m_impl->viewLogWindow->setWindowState(m_impl->viewLogWindow->windowState() & ~Qt::WindowMinimized);
        m_impl->viewLogWindow->raise();
        m_impl->viewLogWindow->activateWindow();
    } else {
        m_impl->viewLogWindow = new ViewLogWindow();
        connect(m_impl->viewLogWindow, &ViewLogWindow::isClosing, this, [this] {
            m_impl->viewLogWindow = nullptr;
            on_windowClose();
        });
        m_impl->openWindows++;
    }
    return m_impl->viewLogWindow;
}

InstanceWindow* Application::showInstanceWindow(MinecraftInstance* instance, const QString& page)
{
    if (!instance) {
        return nullptr;
    }
    auto id = instance->id();
    auto* window = instance->window();

    if (window != nullptr) {
// If the window is minimized on macOS or Windows, activate and bring it up
#ifdef Q_OS_MACOS
        if (window->isMinimized()) {
            window->setWindowState(window->windowState() & ~Qt::WindowMinimized);
        }
#elif defined(Q_OS_WIN)
        if (window->isMinimized()) {
            window->showNormal();
        }
#endif

        window->raise();
        window->activateWindow();
    } else {
        window = new InstanceWindow(instance);
        m_impl->openWindows++;
        connect(window, &InstanceWindow::isClosing, this, [this, instance] {
            instance->setWindow(nullptr);
            if (instance->launchController() != nullptr) {
                instance->launchController()->setParentWidget(m_impl->mainWindow);
            }
            on_windowClose();
        });
        instance->setWindow(window);
    }

    if (!page.isEmpty()) {
        window->selectPage(page);
    }
    if (instance->launchController() != nullptr) {
        instance->launchController()->setParentWidget(window);
    }
    return window;
}

void Application::on_windowClose()
{
    m_impl->openWindows--;
    // quit when there are no more windows.
    if (shouldExitNow()) {
        QApplication::exit(0);
    }
}

void Application::updateProxySettings(const QString& proxyTypeStr,
                                      const QString& addr,
                                      int port,
                                      const QString& user,
                                      const QString& password)
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
    m_impl->network->setProxy(proxy);

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

HttpMetaCache* Application::metacache()
{
    return m_impl->metacache.get();
}

QNetworkAccessManager* Application::network()
{
    return m_impl->network.get();
}

Meta::Index* Application::metadataIndex()
{
    if (!m_impl->metadataIndex) {
        m_impl->metadataIndex.reset(new Meta::Index());
    }
    return m_impl->metadataIndex.get();
}

void Application::updateCapabilities()
{
    m_impl->capabilities = None;
    if (!getMSAClientID().isEmpty()) {
        m_impl->capabilities |= SupportsMSA;
    }
    if (!getFlameAPIKey().isEmpty()) {
        m_impl->capabilities |= SupportsFlame;
    }

#ifdef Q_OS_LINUX
#ifdef ENABLE_GAMEMODE
    if (gamemode_query_status() >= 0) {
        m_impl->capabilities |= SupportsGameMode;
    }
#endif

    if (!LibraryUtils::findMangoHud().isEmpty()) {
        m_impl->capabilities |= SupportsMangoHud;
    }
#endif
}

void Application::detectLibraries()
{
#ifdef Q_OS_LINUX
    m_impl->detectedGLFWPath = LibraryUtils::find(BuildConfig.GLFW_LIBRARY_NAME);
    m_impl->detectedOpenALPath = LibraryUtils::find(BuildConfig.OPENAL_LIBRARY_NAME);
    m_impl->detectedSDLPath = LibraryUtils::find(BuildConfig.SDL_LIBRARY_NAME);
    qDebug() << "Detected native libraries:" << m_impl->detectedGLFWPath << m_impl->detectedOpenALPath << m_impl->detectedSDLPath;
#endif
}

QString Application::getJarPath(const QString& jarFile)
{
    QStringList potentialPaths = {
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        FS::PathCombine(m_impl->rootPath, "share", BuildConfig.LAUNCHER_NAME),
#endif
        FS::PathCombine(m_impl->rootPath, "jars"), FS::PathCombine(QApplication::applicationDirPath(), "jars"),
#if defined(Q_OS_MACOS)
        FS::PathCombine(QApplication::applicationDirPath(), "../../../..", "jars")  // from inside build dir, for debuging
#else
        FS::PathCombine(QApplication::applicationDirPath(), "..", "jars")  // from inside build dir, for debuging
#endif
    };
    for (const auto& p : potentialPaths) {
        QString jarPath = FS::PathCombine(p, jarFile);
        if (QFileInfo(jarPath).isFile()) {
            return jarPath;
        }
    }
    return {};
}

QString Application::getMSAClientID()
{
    QString clientIDOverride = m_impl->settings->get("MSAClientIDOverride").toString();
    if (!clientIDOverride.isEmpty()) {
        return clientIDOverride;
    }

    return BuildConfig.MSA_CLIENT_ID;
}

QString Application::getFlameAPIKey()
{
    QString keyOverride = m_impl->settings->get("FlameKeyOverride").toString();
    if (!keyOverride.isEmpty()) {
        return keyOverride;
    }

    return BuildConfig.FLAME_API_KEY;
}

QString Application::getModrinthAPIToken()
{
    QString tokenOverride = m_impl->settings->get("ModrinthToken").toString();
    if (!tokenOverride.isEmpty()) {
        return tokenOverride;
    }

    return {};
}

QString Application::getUserAgent()
{
    QString uaOverride = m_impl->settings->get("UserAgentOverride").toString();
    if (!uaOverride.isEmpty()) {
        return uaOverride.replace("$LAUNCHER_VER", BuildConfig.printableVersionString());
    }

    return BuildConfig.USER_AGENT;
}

const QString& Application::root() const
{
    return m_impl->rootPath;
}

const QString& Application::dataRoot() const
{
    return m_impl->dataPath;
}

bool Application::isPortable() const
{
    return m_impl->portable;
}

Application::Capabilities Application::capabilities() const
{
    return m_impl->capabilities;
}

void Application::triggerUpdateCheck()
{
    if (m_impl->updater) {
        qDebug() << "Checking for updates.";
        m_impl->updater->setBetaAllowed(false);  // There are no other channels than stable
        m_impl->updater->checkForUpdates();
    } else {
        qDebug() << "Updater not available.";
    }
}

QUrl Application::normalizeImportUrl(const QString& url)
{
    auto localFile = QFileInfo(url);
    if (localFile.exists()) {
        return QUrl::fromLocalFile(localFile.absoluteFilePath());
    }
    return QUrl::fromUserInput(url);
}

QString Application::javaPath()
{
    return m_impl->settings->get("JavaDir").toString();
}

void Application::addQSavePath(const QString& path)
{
    QMutexLocker locker(&m_impl->qsaveResourcesMutex);
    m_impl->qsaveResources[path] = m_impl->qsaveResources.value(path, 0) + 1;
}

void Application::removeQSavePath(const QString& path)
{
    QMutexLocker locker(&m_impl->qsaveResourcesMutex);
    auto count = m_impl->qsaveResources.value(path, 0) - 1;
    if (count <= 0) {
        m_impl->qsaveResources.remove(path);
    } else {
        m_impl->qsaveResources[path] = count;
    }
}

bool Application::checkQSavePath(const QString& path)
{
    QMutexLocker locker(&m_impl->qsaveResourcesMutex);
    return std::ranges::any_of(m_impl->qsaveResources.keys(), [&path, this](const auto& partialPath) {
        return path.startsWith(partialPath) && m_impl->qsaveResources.value(partialPath, 0) > 0;
    });
}

QFile* Application::logFile()
{
    return m_impl->logFile.get();
}

LogModel* Application::logModel()
{
    return m_impl->logModel.get();
}

const QString& Application::detectedGLFWPath() const
{
    return m_impl->detectedGLFWPath;
}

const QString& Application::detectedOpenALPath() const
{
    return m_impl->detectedOpenALPath;
}

const QString& Application::detectedSDLPath() const
{
    return m_impl->detectedSDLPath;
}
