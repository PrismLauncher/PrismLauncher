// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#include "PrismUpdater.h"
#include "BuildConfig.h"
#include "ui/dialogs/ProgressDialog.h"

#include <cstdlib>
#include <iostream>

#include <QAccessible>
#include <QCommandLineParser>
#include <QNetworkReply>

#include <QDebug>

#include <QMessageBox>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QProcess>
#include <memory>

#include <qglobal.h>
#include <sys.h>

#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>
#include <iostream>
#endif

// Snippet from https://github.com/gulrak/filesystem#using-it-as-single-file-header

#ifdef __APPLE__
#include <Availability.h>  // for deployment target to support pre-catalina targets without std::fs
#endif                     // __APPLE__

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || (defined(__cplusplus) && __cplusplus >= 201703L)) && defined(__has_include)
#if __has_include(<filesystem>) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif  // MacOS min version check
#endif  // Other OSes version check

#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

#include <DesktopServices.h>

#include "updater/prismupdater/UpdaterDialogs.h"

#include "FileSystem.h"
#include "Json.h"
#include "StringUtils.h"

#include "net/Download.h"
#include "net/RawHeaderProxy.h"

#include <MMCZip.h>

/** output to the log file */
void appDebugOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static std::mutex loggerMutex;
    const std::lock_guard<std::mutex> lock(loggerMutex);  // synchronized, QFile logFile is not thread-safe

    QString out = qFormatLogMessage(type, context, msg);
    out += QChar::LineFeed;

    PrismUpdaterApp* app = static_cast<PrismUpdaterApp*>(QCoreApplication::instance());
    app->logFile->write(out.toUtf8());
    app->logFile->flush();
    if (app->logToConsole) {
        QTextStream(stderr) << out.toLocal8Bit();
        fflush(stderr);
    }
}

#if defined Q_OS_WIN32

// taken from https://stackoverflow.com/a/25927081
// getting a proper output to console with redirection support on windows is apearently hell
void BindCrtHandlesToStdHandles(bool bindStdIn, bool bindStdOut, bool bindStdErr)
{
    // Re-initialize the C runtime "FILE" handles with clean handles bound to "nul". We do this because it has been
    // observed that the file number of our standard handle file objects can be assigned internally to a value of -2
    // when not bound to a valid target, which represents some kind of unknown internal invalid state. In this state our
    // call to "_dup2" fails, as it specifically tests to ensure that the target file number isn't equal to this value
    // before allowing the operation to continue. We can resolve this issue by first "re-opening" the target files to
    // use the "nul" device, which will place them into a valid state, after which we can redirect them to our target
    // using the "_dup2" function.
    if (bindStdIn) {
        FILE* dummyFile;
        freopen_s(&dummyFile, "nul", "r", stdin);
    }
    if (bindStdOut) {
        FILE* dummyFile;
        freopen_s(&dummyFile, "nul", "w", stdout);
    }
    if (bindStdErr) {
        FILE* dummyFile;
        freopen_s(&dummyFile, "nul", "w", stderr);
    }

    // Redirect unbuffered stdin from the current standard input handle
    if (bindStdIn) {
        HANDLE stdHandle = GetStdHandle(STD_INPUT_HANDLE);
        if (stdHandle != INVALID_HANDLE_VALUE) {
            int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
            if (fileDescriptor != -1) {
                FILE* file = _fdopen(fileDescriptor, "r");
                if (file != NULL) {
                    int dup2Result = _dup2(_fileno(file), _fileno(stdin));
                    if (dup2Result == 0) {
                        setvbuf(stdin, NULL, _IONBF, 0);
                    }
                }
            }
        }
    }

    // Redirect unbuffered stdout to the current standard output handle
    if (bindStdOut) {
        HANDLE stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (stdHandle != INVALID_HANDLE_VALUE) {
            int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
            if (fileDescriptor != -1) {
                FILE* file = _fdopen(fileDescriptor, "w");
                if (file != NULL) {
                    int dup2Result = _dup2(_fileno(file), _fileno(stdout));
                    if (dup2Result == 0) {
                        setvbuf(stdout, NULL, _IONBF, 0);
                    }
                }
            }
        }
    }

    // Redirect unbuffered stderr to the current standard error handle
    if (bindStdErr) {
        HANDLE stdHandle = GetStdHandle(STD_ERROR_HANDLE);
        if (stdHandle != INVALID_HANDLE_VALUE) {
            int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
            if (fileDescriptor != -1) {
                FILE* file = _fdopen(fileDescriptor, "w");
                if (file != NULL) {
                    int dup2Result = _dup2(_fileno(file), _fileno(stderr));
                    if (dup2Result == 0) {
                        setvbuf(stderr, NULL, _IONBF, 0);
                    }
                }
            }
        }
    }

    // Clear the error state for each of the C++ standard stream objects. We need to do this, as attempts to access the
    // standard streams before they refer to a valid target will cause the iostream objects to enter an error state. In
    // versions of Visual Studio after 2005, this seems to always occur during startup regardless of whether anything
    // has been read from or written to the targets or not.
    if (bindStdIn) {
        std::wcin.clear();
        std::cin.clear();
    }
    if (bindStdOut) {
        std::wcout.clear();
        std::cout.clear();
    }
    if (bindStdErr) {
        std::wcerr.clear();
        std::cerr.clear();
    }
}
#endif

PrismUpdaterApp::PrismUpdaterApp(int& argc, char** argv) : QApplication(argc, argv)
{
#if defined Q_OS_WIN32
    // attach the parent console
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        BindCrtHandlesToStdHandles(true, true, true);
        consoleAttached = true;
    }
#endif
    setOrganizationName(BuildConfig.LAUNCHER_NAME);
    setOrganizationDomain(BuildConfig.LAUNCHER_DOMAIN);
    setApplicationName(BuildConfig.LAUNCHER_NAME + "Updater");
    setApplicationVersion(BuildConfig.printableVersionString() + "\n" + BuildConfig.GIT_COMMIT);

    // Command line parsing
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("An auto-updater for Prism Launcher"));

    parser.addOptions(
        { { { "d", "dir" }, tr("Use a custom path as application root (use '.' for current directory)."), tr("directory") },
          { { "V", "prism-version" },
            tr("Use this version as the installed launcher version. (provided because stdout can not be reliably captured on windows)"),
            tr("installed launcher version") },
          { { "I", "install-version" }, "Install a specific version.", tr("version name") },
          { { "U", "update-url" }, tr("Update from the specified repo."), tr("github repo url") },
          { { "c", "check-only" },
            tr("Only check if an update is needed. Exit status 100 if true, 0 if false (or non 0 if there was an error).") },
          { { "F", "force" }, tr("Force an update, even if one is not needed.") },
          { { "l", "list" }, tr("List available releases.") },
          { "debug", tr("Log debug to console.") },
          { { "S", "select-ui" }, tr("Select the version to install with a GUI.") },
          { { "D", "allow-downgrade" }, tr("Allow the updater to downgrade to previous versions.") } });

    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(arguments());

    logToConsole = parser.isSet("debug");

    auto updater_executable = QCoreApplication::applicationFilePath();

    if (BuildConfig.BUILD_PLATFORM.toLower() == "macos")
        showFatalErrorMessage(tr("MacOS Not Supported"), tr("The updater does not support installations on MacOS"));

    if (updater_executable.startsWith("/tmp/.mount_")) {
        m_isAppimage = true;
        m_appimagePath = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));
        if (m_appimagePath.isEmpty()) {
            showFatalErrorMessage(tr("Unsupported Installation"),
                                  tr("Updater is running as misconfigured AppImage? ($APPIMAGE environment variable is missing)"));
        }
    }

    m_isFlatpak = DesktopServices::isFlatpak();

    QString prism_executable = QCoreApplication::applicationDirPath() + "/" + BuildConfig.LAUNCHER_APP_BINARY_NAME;
#if defined Q_OS_WIN32
    prism_executable += ".exe";
#endif

    if (!QFileInfo(prism_executable).isFile()) {
        showFatalErrorMessage(tr("Unsupported Installation"), tr("The updater can not find the main executable."));
    }

    m_prismExecutable = prism_executable;

    auto prism_update_url = parser.value("update-url");
    if (prism_update_url.isEmpty())
        prism_update_url = "https://github.com/PrismLauncher/PrismLauncher";
    m_prismRepoUrl = QUrl::fromUserInput(prism_update_url);

    m_checkOnly = parser.isSet("check-only");
    m_forceUpdate = parser.isSet("force");
    m_printOnly = parser.isSet("list");
    auto user_version = parser.value("install-version");
    if (!user_version.isEmpty()) {
        m_userSelectedVersion = Version(user_version);
    }
    m_selectUI = parser.isSet("select-ui");
    m_allowDowngrade = parser.isSet("allow-downgrade");

    auto version = parser.value("prism-version");
    if (!version.isEmpty()) {
        if (version.contains('-')) {
            auto index = version.indexOf('-');
            m_prsimVersionChannel = version.mid(index + 1);
            version = version.left(index);
        } else {
            m_prsimVersionChannel = "stable";
        }
        auto version_parts = version.split('.');
        m_prismVersionMajor = version_parts.takeFirst().toInt();
        m_prismVersionMinor = version_parts.takeFirst().toInt();
    }

    QString origCwdPath = QDir::currentPath();
    QString binPath = applicationDirPath();

    {  // find data director
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
    // change folder
    QString dirParam = parser.value("dir");
    if (!dirParam.isEmpty()) {
        // the dir param. it makes prism launcher data path point to whatever the user specified
        // on command line
        adjustedBy = "Command line";
        m_dataPath = dirParam;
    } else {
        QDir foo(FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), ".."));
        m_dataPath = foo.absolutePath();
        adjustedBy = "Persistent data path";

#ifndef Q_OS_MACOS
        if (QFile::exists(FS::PathCombine(m_rootPath, "portable.txt"))) {
            m_dataPath = m_rootPath;
            adjustedBy = "Portable data path";
            m_isPortable = true;
        }
#endif
    }

    {  // setup logging
        static const QString logBase = BuildConfig.LAUNCHER_NAME + "Updater" + (m_checkOnly ? "-CheckOnly" : "") + "-%0.log";
        auto moveFile = [](const QString& oldName, const QString& newName) {
            QFile::remove(newName);
            QFile::copy(oldName, newName);
            QFile::remove(oldName);
        };

        moveFile(logBase.arg(1), logBase.arg(2));
        moveFile(logBase.arg(0), logBase.arg(1));

        logFile = std::unique_ptr<QFile>(new QFile(logBase.arg(0)));
        if (!logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            showFatalErrorMessage(tr("The launcher data folder is not writable!"),
                                  tr("The launcher couldn't create a log file - the data folder is not writable.\n"
                                     "\n"
                                     "Make sure you have write permissions to the data folder.\n"
                                     "(%1)\n"
                                     "\n"
                                     "The launcher cannot continue until you fix this problem.")
                                      .arg(m_dataPath));
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
        auto logRulesPath = FS::PathCombine(m_dataPath, logRulesFile);

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
            logRulesPath = FS::PathCombine(m_rootPath, logRulesFile);
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

    {  // log debug program info
        qDebug() << qPrintable(BuildConfig.LAUNCHER_DISPLAYNAME) << "Updater"
                 << ", (c) 2022-2023 " << qPrintable(QString(BuildConfig.LAUNCHER_COPYRIGHT).replace("\n", ", "));
        qDebug() << "Version                    : " << BuildConfig.printableVersionString();
        qDebug() << "Git commit                 : " << BuildConfig.GIT_COMMIT;
        qDebug() << "Git refspec                : " << BuildConfig.GIT_REFSPEC;
        if (adjustedBy.size()) {
            qDebug() << "Work dir before adjustment : " << origCwdPath;
            qDebug() << "Work dir after adjustment  : " << QDir::currentPath();
            qDebug() << "Adjusted by                : " << adjustedBy;
        } else {
            qDebug() << "Work dir                   : " << QDir::currentPath();
        }
        qDebug() << "Binary path                : " << binPath;
        qDebug() << "Application root path      : " << m_rootPath;
        qDebug() << "<> Paths set.";
    }

    {  // network
        m_network = makeShared<QNetworkAccessManager>(new QNetworkAccessManager());
        qDebug() << "Detecting proxy settings...";
        QNetworkProxy proxy = QNetworkProxy::applicationProxy();
        m_network->setProxy(proxy);
    }

    QMetaObject::invokeMethod(this, &PrismUpdaterApp::loadReleaseList, Qt::QueuedConnection);
}

PrismUpdaterApp::~PrismUpdaterApp()
{
    // Shut down logger by setting the logger function to nothing
    qInstallMessageHandler(nullptr);
    qDebug() << "updater shutting down";

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

void PrismUpdaterApp::fail(const QString& reason)
{
    qCritical() << qPrintable(reason);
    m_status = Failed;
    exit(1);
}

void PrismUpdaterApp::abort(const QString& reason)
{
    qCritical() << qPrintable(reason);
    m_status = Aborted;
    exit(2);
}

void PrismUpdaterApp::showFatalErrorMessage(const QString& title, const QString& content)
{
    m_status = Failed;
    auto msgBox = new QMessageBox();
    msgBox->setWindowTitle(title);
    msgBox->setText(content);
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setDefaultButton(QMessageBox::Ok);
    msgBox->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
    msgBox->setIcon(QMessageBox::Critical);
    msgBox->exec();
    exit(1);
}

void PrismUpdaterApp::run()
{
    qDebug() << "found" << m_releases.length() << "releases on github";
    qDebug() << "loading exe at " << m_prismExecutable;

    if (m_printOnly) {
        printReleases();
        m_status = Succeeded;
        return exit(0);
    }

    if (!loadPrismVersionFromExe(m_prismExecutable)) {
        m_prismVersion = BuildConfig.printableVersionString();
        m_prismVersionMajor = BuildConfig.VERSION_MAJOR;
        m_prismVersionMinor = BuildConfig.VERSION_MINOR;
        m_prsimVersionChannel = BuildConfig.VERSION_CHANNEL;
        m_prismGitCommit = BuildConfig.GIT_COMMIT;
    }
    m_status = Succeeded;

    qDebug() << "Executable reports as:" << m_prismBinaryName << "version:" << m_prismVersion;
    qDebug() << "Version major:" << m_prismVersionMajor;
    qDebug() << "Version minor:" << m_prismVersionMinor;
    qDebug() << "Version channel:" << m_prsimVersionChannel;
    qDebug() << "Git Commit:" << m_prismGitCommit;

    auto latest = getLatestRelease();
    qDebug() << "Latest release" << latest.version;
    auto need_update = needUpdate(latest);

    if (m_checkOnly) {
        if (need_update)
            return exit(100);
        else
            return exit(0);
    }

    if (m_isFlatpak) {
        showFatalErrorMessage(tr("Updating flatpack not supported"), tr("Actions outside of checking if an update is available are not "
                                                                        "supported when running the flatpak version of Prism Launcher."));
        return;
    }
    if (m_isAppimage) {
        bool result = true;
        if (need_update)
            result = callAppImageUpdate();
        return exit(result ? 0 : 1);
    }

    if (BuildConfig.BUILD_PLATFORM.toLower() == "linux" && !m_isPortable) {
        showFatalErrorMessage(tr("Updating Not Supported"),
                              tr("Updating non-portable linux installations is not supported. Please use your system package manager"));
        return;
    }

    if (need_update || m_forceUpdate || !m_userSelectedVersion.isEmpty()) {
        GitHubRelease update_release = latest;
        if (!m_userSelectedVersion.isEmpty()) {
            bool found = false;
            for (auto rls : m_releases) {
                if (rls.version == m_userSelectedVersion) {
                    found = true;
                    update_release = rls;
                    break;
                }
            }
            if (!found) {
                showFatalErrorMessage(
                    "No release for version!",
                    QString("Can not find a github release for specified version %1").arg(m_userSelectedVersion.toString()));
                return;
            }
        } else if (m_selectUI) {
            update_release = selectRelease();
            if (!update_release.isValid()) {
                showFatalErrorMessage("No version selected.", "No version was selected.");
                return;
            }
        }

        performUpdate(update_release);
    }

    exit(0);
}

void PrismUpdaterApp::printReleases()
{
    for (auto release : m_releases) {
        std::cout << release.name.toStdString() << " Version: " << release.tag_name.toStdString() << std::endl;
    }
}

QList<GitHubRelease> PrismUpdaterApp::nonDraftReleases()
{
    QList<GitHubRelease> nonDraft;
    for (auto rls : m_releases) {
        if (rls.isValid() && !rls.draft)
            nonDraft.append(rls);
    }
    return nonDraft;
}

QList<GitHubRelease> PrismUpdaterApp::newerReleases()
{
    QList<GitHubRelease> newer;
    for (auto rls : nonDraftReleases()) {
        if (rls.version > m_prismVersion)
            newer.append(rls);
    }
    return newer;
}

GitHubRelease PrismUpdaterApp::selectRelease()
{
    QList<GitHubRelease> releases;

    if (m_allowDowngrade) {
        releases = nonDraftReleases();
    } else {
        releases = newerReleases();
    }

    if (releases.isEmpty())
        return {};

    SelectReleaseDialog dlg(Version(m_prismVersion), releases);
    auto result = dlg.exec();

    if (result == QDialog::Rejected) {
        return {};
    }
    GitHubRelease release = dlg.selectedRelease();

    return release;
}

QList<GitHubReleaseAsset> PrismUpdaterApp::validReleaseArtifacts(const GitHubRelease& release)
{
    QList<GitHubReleaseAsset> valid;

    qDebug() << "Selecting best asset from" << release.tag_name << "for platform" << BuildConfig.BUILD_PLATFORM
             << "portable:" << m_isPortable;
    if (BuildConfig.BUILD_PLATFORM.isEmpty())
        qWarning() << "Build platform is not set!";
    for (auto asset : release.assets) {
        if (!m_isAppimage && asset.name.toLower().endsWith("appimage"))
            continue;
        else if (m_isAppimage && !asset.name.toLower().endsWith("appimage"))
            continue;
        auto asset_name = asset.name.toLower();
        auto platform = BuildConfig.BUILD_PLATFORM.toLower();
        auto system_is_arm = QSysInfo::buildCpuArchitecture().contains("arm64");
        auto asset_is_arm = asset_name.contains("arm64");
        auto asset_is_archive = asset_name.endsWith(".zip") || asset_name.endsWith(".tar.gz");

        bool for_platform = !platform.isEmpty() && asset_name.contains(platform);
        bool for_portable = asset_name.contains("portable");
        if (for_platform && asset_name.contains("legacy") && !platform.contains("legacy"))
            for_platform = false;
        if (for_platform && ((asset_is_arm && !system_is_arm) || (!asset_is_arm && system_is_arm)))
            for_platform = false;
        if (for_platform && platform.contains("windows") && !m_isPortable && asset_is_archive)
            for_platform = false;

        if (((m_isPortable && for_portable) || (!m_isPortable && !for_portable)) && for_platform) {
            valid.append(asset);
        }
    }
    return valid;
}

GitHubReleaseAsset PrismUpdaterApp::selectAsset(const QList<GitHubReleaseAsset>& assets)
{
    SelectReleaseAssetDialog dlg(assets);
    auto result = dlg.exec();

    if (result == QDialog::Rejected) {
        return {};
    }

    GitHubReleaseAsset asset = dlg.selectedAsset();
    return asset;
}

void PrismUpdaterApp::performUpdate(const GitHubRelease& release)
{
    m_install_release = release;
    qDebug() << "Updating to" << release.tag_name;
    auto valid_assets = validReleaseArtifacts(release);
    qDebug() << "valid release assets:" << valid_assets;

    GitHubReleaseAsset selected_asset;
    if (valid_assets.isEmpty()) {
        return showFatalErrorMessage(
            tr("No Valid Release Assets"),
            tr("Github release %1 has no valid assets for this platform: %2")
                .arg(release.tag_name)
                .arg(tr("%1 portable: %2").arg(BuildConfig.BUILD_PLATFORM).arg(m_isPortable ? tr("yes") : tr("no"))));
    } else if (valid_assets.length() > 1) {
        selected_asset = selectAsset(valid_assets);
    } else {
        selected_asset = valid_assets.takeFirst();
    }

    if (!selected_asset.isValid()) {
        return showFatalErrorMessage(tr("No version selected."), tr("No version was selected."));
    }

    qDebug() << "will install" << selected_asset;
    auto file = downloadAsset(selected_asset);

    if (!file.exists()) {
        return showFatalErrorMessage(tr("Failed to Download"), tr("Failed to download the selected asset."));
    }

    performInstall(file);
}

QFileInfo PrismUpdaterApp::downloadAsset(const GitHubReleaseAsset& asset)
{
    auto temp_dir = QDir::tempPath();
    auto file_url = QUrl(asset.browser_download_url);
    auto out_file_path = FS::PathCombine(temp_dir, file_url.fileName());

    qDebug() << "downloading" << file_url << "to" << out_file_path;
    auto download = Net::Download::makeFile(file_url, out_file_path);
    download->setNetwork(m_network);
    auto progress_dialog = ProgressDialog();

    if (progress_dialog.execWithTask(download.get()) == QDialog::Rejected)
        showFatalErrorMessage(tr("Download Aborted"), tr("Download of %1 aborted by user").arg(file_url.toString()));

    qDebug() << "download complete";

    QFileInfo out_file(out_file_path);
    return out_file;
}

bool PrismUpdaterApp::callAppImageUpdate()
{
    QProcess proc = QProcess();
    proc.setProgram("AppImageUpdate");
    proc.setArguments({ QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE")) });
    return proc.startDetached();
}

void PrismUpdaterApp::clearUpdateLog()
{
    auto update_log_path = FS::PathCombine(m_dataPath, "prism_launcher_update.log");
    QFile::remove(update_log_path);
}

void PrismUpdaterApp::logUpdate(const QString& msg)
{
    qDebug() << qUtf8Printable(msg);
    auto update_log_path = FS::PathCombine(m_dataPath, "prism_launcher_update.log");
    FS::append(update_log_path, QStringLiteral("%1\n").arg(msg).toUtf8());
}

void PrismUpdaterApp::performInstall(QFileInfo file)
{
    qDebug() << "starting install";
    auto update_lock_path = FS::PathCombine(m_dataPath, ".prism_launcher_update.lock");
    FS::write(update_lock_path, QStringLiteral("FROM=%1\nTO=%2\n").arg(m_prismVersion).arg(m_install_release.tag_name).toUtf8());
    clearUpdateLog();

    logUpdate(tr("Updating from %1 to %2").arg(m_prismVersion).arg(m_install_release.tag_name));
    if (m_isPortable || file.suffix().toLower() == "zip") {
        logUpdate(tr("Updating portable install at %1").arg(applicationDirPath()));
        unpackAndInstall(file);
    } else {
        logUpdate(tr("Running installer file at %1").arg(file.absoluteFilePath()));
        QProcess proc = QProcess();
        proc.setProgram(file.absoluteFilePath());
        bool result = proc.startDetached();
        logUpdate(tr("Process start result: %1").arg(result ? tr("yes") : tr("no")));
        exit(result ? 0 : 1);
    }
}

void PrismUpdaterApp::unpackAndInstall(QFileInfo archive)
{
    logUpdate(tr("Backing up install"));
    backupAppDir();

    if (auto loc = unpackArchive(archive)) {
        auto marker_file_path = loc.value().absoluteFilePath(".prism_launcher_updater_unpack.marker");
        FS::write(marker_file_path, applicationDirPath().toUtf8());
        auto new_updater_path = loc.value().absoluteFilePath("prismlauncher-updater");
#if defined Q_OS_WIN32
        new_updater_path.append(".exe");
#endif
        logUpdate(tr("Starting new updater at '%1'").arg(new_updater_path));
        QProcess proc = QProcess();
        proc.startDetached(new_updater_path, {}, loc.value().absolutePath());
        if (!proc.waitForStarted(5000)) {
            logUpdate(tr("Failed to launch '%1' %2").arg(new_updater_path).arg(proc.errorString()));
            return exit(10);
        }
        return exit();  // up to the new updater now
    }
    return exit(1);  // unpack failure
}

void PrismUpdaterApp::backupAppDir()
{
    auto manifest_path = FS::PathCombine(applicationDirPath(), "manifest.txt");
    QFileInfo manifest(manifest_path);

    QStringList file_list;
    if (manifest.isFile()) {
        // load manifest from file

        logUpdate(tr("Reading manifest from %1").arg(manifest.absoluteFilePath()));
        try {
            auto contents = QString::fromUtf8(FS::read(manifest.absoluteFilePath()));
            file_list.append(contents.split('\n'));
        } catch (FS::FileSystemException) {
        }
    }

    if (file_list.isEmpty()) {
        // best guess
        if (BuildConfig.BUILD_PLATFORM.toLower() == "linux") {
            file_list.append({ "PrismLauncher", "bin/*", "share/*", "lib/*" });
        } else {  // windows by process of elimination
            file_list.append({
                "jars/*",
                "prismlauncher.exe",
                "prismlauncher_filelink.exe",
                "qtlogging.ini",
                "imageformats/*",
                "iconengines/*",
                "platforms/*",
                "styles/*",
                "styles/*",
                "tls/*",
                "qt.conf",
                "Qt*.dll",
            });
        }
        file_list.append("portable.txt");
        logUpdate("manifest.txt empty or missing. making best guess at files to back up.");
    }
    logUpdate(tr("Backing up:\n  %1").arg(file_list.join(",\n  ")));

    QDir app_dir = QCoreApplication::applicationDirPath();
    auto backup_dir = FS::PathCombine(
        app_dir.absolutePath(), QStringLiteral("backup_") +
                                    QString(m_prismVersion).replace(QRegularExpression("[" + QRegularExpression::escape("\\/:*?\"<>|") + "]"), QString("_")) +
                                    "-" + m_prismGitCommit);
    FS::ensureFolderPathExists(backup_dir);

    for (auto glob : file_list) {
        QDirIterator iter(app_dir.absolutePath(), QStringList({ glob }), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
        while (iter.hasNext()) {
            auto to_bak_file = iter.next();
            auto rel_path = app_dir.relativeFilePath(to_bak_file);
            auto bak_path = FS::PathCombine(backup_dir, rel_path);

            if (QFileInfo(to_bak_file).isFile()) {
                logUpdate(tr("Backing up and then removing %1").arg(to_bak_file));
                FS::ensureFilePathExists(bak_path);
                auto result = FS::copy(to_bak_file, bak_path)();
                if (!result) {
                    logUpdate(tr("Failed to backup %1 to %2").arg(to_bak_file).arg(bak_path));
                } else {
                    if (!FS::deletePath(to_bak_file))
                        logUpdate(tr("Failed to remove %1").arg(to_bak_file));
                }
            }
        }
    }
}

std::optional<QDir> PrismUpdaterApp::unpackArchive(QFileInfo archive)
{
    auto temp_extract_path = FS::PathCombine(m_dataPath, "prism_launcher_update_release");
    FS::ensureFolderPathExists(temp_extract_path);
    auto tmp_extract_dir = QDir(temp_extract_path);

    if (archive.fileName().endsWith(".zip")) {
        auto result = MMCZip::extractDir(archive.absoluteFilePath(), tmp_extract_dir.absolutePath());
        if (result) {
            logUpdate(tr("Extracted the following to \"%1\":\n  %2").arg(tmp_extract_dir.absolutePath()).arg(result->join("\n  ")));
        } else {
            logUpdate(tr("Failed to extract %1 to %2").arg(archive.absoluteFilePath()).arg(tmp_extract_dir.absolutePath()));
            showFatalErrorMessage("Failed to extract archive",
                                  tr("Failed to extract %1 to %2").arg(archive.absoluteFilePath()).arg(tmp_extract_dir.absolutePath()));
            return std::nullopt;
        }

    } else if (archive.fileName().endsWith(".tar.gz")) {
        QString cmd = "tar";
        QStringList args = { "-xvf", archive.absoluteFilePath(), "-C", tmp_extract_dir.absolutePath() };
        logUpdate(tr("Running: `%1 %2`").arg(cmd).arg(args.join(" ")));
        QProcess proc = QProcess();
        proc.start(cmd, args);
        if (!proc.waitForStarted(5000)) {  // wait 5 seconds to start
            auto msg = tr("Failed to launcher child process \"%1 %2\".").arg(cmd).arg(args.join(" "));
            logUpdate(msg);
            showFatalErrorMessage(tr("Failed extract archive"), msg);
            return std::nullopt;
        }
        auto result = proc.waitForFinished(5000);
        auto out = proc.readAll();
        logUpdate(out);
        if (!result) {
            auto msg = tr("Child process \"%1 %2\" failed.").arg(cmd).arg(args.join(" "));
            logUpdate(msg);
            showFatalErrorMessage(tr("Failed to extract archive"), msg);
            return std::nullopt;
        }

    } else {
        logUpdate(tr("Unknown archive format for %1").arg(archive.absoluteFilePath()));
        showFatalErrorMessage("Can not extract", QStringLiteral("Unknown archive format %1").arg(archive.absoluteFilePath()));
        return std::nullopt;
    }

    return tmp_extract_dir;
}

bool PrismUpdaterApp::loadPrismVersionFromExe(const QString& exe_path)
{
    QProcess proc = QProcess();
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.setReadChannel(QProcess::StandardOutput);
    proc.start(exe_path, { "-v" });
    if (!proc.waitForStarted(5000)) {
        showFatalErrorMessage(tr("Failed to Check Version"), tr("Failed to launcher child launcher process to read version."));
        return false;
    }  // wait 5 seconds to start
    if (!proc.waitForFinished(5000)) {
        showFatalErrorMessage(tr("Failed to Check Version"), tr("Child launcher process failed."));
        return false;
    }
    auto out = proc.readAll();
    auto lines = out.split('\n');
    if (lines.length() < 2)
        return false;
    auto first = lines.takeFirst();
    auto first_parts = first.split(' ');
    if (first_parts.length() < 2)
        return false;
    m_prismBinaryName = first_parts.takeFirst();
    auto version = first_parts.takeFirst();
    m_prismVersion = version;
    if (version.contains('-')) {
        auto index = version.indexOf('-');
        m_prsimVersionChannel = version.mid(index + 1);
        version = version.left(index);
    } else {
        m_prsimVersionChannel = "stable";
    }
    auto version_parts = version.split('.');
    m_prismVersionMajor = version_parts.takeFirst().toInt();
    m_prismVersionMinor = version_parts.takeFirst().toInt();
    m_prismGitCommit = lines.takeFirst().simplified();
    return true;
}

void PrismUpdaterApp::loadReleaseList()
{
    auto github_repo = m_prismRepoUrl;
    if (github_repo.host() != "github.com")
        return fail("updating from a non github url is not supported");

    auto path_parts = github_repo.path().split('/');
    path_parts.removeFirst();  // empty segment from leading /
    auto repo_owner = path_parts.takeFirst();
    auto repo_name = path_parts.takeFirst();
    auto api_url = QString("https://api.github.com/repos/%1/%2/releases").arg(repo_owner, repo_name);

    qDebug() << "Fetching release list from" << api_url;

    downloadReleasePage(api_url, 1);
}

void PrismUpdaterApp::downloadReleasePage(const QString& api_url, int page)
{
    int per_page = 30;
    auto page_url = QString("%1?per_page=%2&page=%3").arg(api_url).arg(QString::number(per_page)).arg(QString::number(page));
    auto response = std::make_shared<QByteArray>();
    auto download = Net::Download::makeByteArray(page_url, response.get());
    download->setNetwork(m_network);
    m_current_url = page_url;

    auto github_api_headers = new Net::RawHeaderProxy();
    github_api_headers->addHeaders({
        { "Accept", "application/vnd.github+json" },
        { "X-GitHub-Api-Version", "2022-11-28" },
    });
    download->addHeaderProxy(github_api_headers);

    connect(download.get(), &Net::Download::succeeded, this, [this, response, per_page, api_url, page]() {
        int num_found = parseReleasePage(response.get());
        if (!(num_found < per_page)) {  // there may be more, fetch next page
            downloadReleasePage(api_url, page + 1);
        } else {
            run();
        }
    });
    connect(download.get(), &Net::Download::failed, this, &PrismUpdaterApp::downloadError);

    m_current_task.reset(download);
    connect(download.get(), &Net::Download::finished, this, [this]() {
        qDebug() << "Download" << m_current_task->getUid().toString() << "finished";
        m_current_task.reset();
        m_current_url = "";
    });

    QCoreApplication::processEvents();

    QMetaObject::invokeMethod(download.get(), &Task::start, Qt::QueuedConnection);
}

int PrismUpdaterApp::parseReleasePage(const QByteArray* response)
{
    if (response->isEmpty())  // empty page
        return 0;
    int num_releases = 0;
    try {
        auto doc = Json::requireDocument(*response);
        auto release_list = Json::requireArray(doc);
        for (auto release_json : release_list) {
            auto release_obj = Json::requireObject(release_json);

            GitHubRelease release = {};
            release.id = Json::requireInteger(release_obj, "id");
            release.name = Json::ensureString(release_obj, "name");
            release.tag_name = Json::requireString(release_obj, "tag_name");
            release.created_at = QDateTime::fromString(Json::requireString(release_obj, "created_at"), Qt::ISODate);
            release.published_at = QDateTime::fromString(Json::ensureString(release_obj, "published_at"), Qt::ISODate);
            release.draft = Json::requireBoolean(release_obj, "draft");
            release.prerelease = Json::requireBoolean(release_obj, "prerelease");
            release.body = Json::ensureString(release_obj, "body");
            release.version = Version(release.tag_name);

            auto release_assets_obj = Json::requireArray(release_obj, "assets");
            for (auto asset_json : release_assets_obj) {
                auto asset_obj = Json::requireObject(asset_json);
                GitHubReleaseAsset asset = {};
                asset.id = Json::requireInteger(asset_obj, "id");
                asset.name = Json::requireString(asset_obj, "name");
                asset.label = Json::ensureString(asset_obj, "label");
                asset.content_type = Json::requireString(asset_obj, "content_type");
                asset.size = Json::requireInteger(asset_obj, "size");
                asset.created_at = QDateTime::fromString(Json::requireString(asset_obj, "created_at"), Qt::ISODate);
                asset.updated_at = QDateTime::fromString(Json::requireString(asset_obj, "updated_at"), Qt::ISODate);
                asset.browser_download_url = Json::requireString(asset_obj, "browser_download_url");
                release.assets.append(asset);
            }
            m_releases.append(release);
            num_releases++;
        }
    } catch (Json::JsonException& e) {
        auto err_msg =
            QString("Failed to parse releases from github: %1\n%2").arg(e.what()).arg(QString::fromStdString(response->toStdString()));
        fail(err_msg);
    }
    return num_releases;
}

GitHubRelease PrismUpdaterApp::getLatestRelease()
{
    GitHubRelease latest;
    for (auto release : m_releases) {
        if (!latest.isValid() || (!release.draft && release.version > latest.version)) {
            latest = release;
        }
    }
    return latest;
}

bool PrismUpdaterApp::needUpdate(const GitHubRelease& release)
{
    auto current_ver = Version(QString("%1.%2").arg(QString::number(m_prismVersionMajor)).arg(QString::number(m_prismVersionMinor)));
    return current_ver < release.version;
}

void PrismUpdaterApp::downloadError(QString reason)
{
    fail(QString("Network request Failed: %1 with reason %2").arg(m_current_url).arg(reason));
}
