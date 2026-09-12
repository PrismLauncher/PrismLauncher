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

#include <iostream>

#include <QDebug>

#include <QAccessible>
#include <QCommandLineParser>
#include <QDirListing>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <memory>

#include "DesktopServices.h"

#include "updater/prismupdater/UpdaterDialogs.h"

#include "FileSystem.h"
#include "Json.h"
#include "StringUtils.h"

#include "net/RawHeaderProxy.h"
#include "net/Request.h"

#include "MMCZip.h"

namespace {

/** output to the log file */
void appDebugOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static std::mutex s_loggerMutex;
    const std::lock_guard<std::mutex> lock(s_loggerMutex);  // synchronized, QFile logFile is not thread-safe

    QString out = qFormatLogMessage(type, context, msg);
    out += QChar::LineFeed;

    auto* app = static_cast<PrismUpdaterApp*>(QCoreApplication::instance());
    app->logFile->write(out.toUtf8());
    app->logFile->flush();
    if (app->logToConsole) {
        QTextStream(stderr) << out.toLocal8Bit();
        fflush(stderr);
    }
}
}  // namespace

PrismUpdaterApp::PrismUpdaterApp(int& argc, char** argv) : QApplication(argc, argv)
{
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
          { { "p", "pre-release" }, tr("Allow updating to pre-release releases") },
          { { "F", "force" }, tr("Force an update, even if one is not needed.") },
          { { "l", "list" }, tr("List available releases.") },
          { "debug", tr("Log debug to console.") },
          { { "S", "select-ui" }, tr("Select the version to install with a GUI.") },
          { { "D", "allow-downgrade" }, tr("Allow the updater to downgrade to previous versions.") } });

    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(arguments());

    logToConsole = parser.isSet("debug");

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
#ifndef Q_OS_MACOS
        if (QDir(FS::PathCombine(m_rootPath, "UserData")).exists()) {
            m_isPortable = true;
        }
        if (QFile::exists(FS::PathCombine(m_rootPath, "portable.txt"))) {
            m_isPortable = true;
        }
#endif
    } else if (auto dataDirEnv =
                   QProcessEnvironment::systemEnvironment().value(QString("%1_DATA_DIR").arg(BuildConfig.LAUNCHER_NAME.toUpper()));
               !dataDirEnv.isEmpty()) {
        adjustedBy = "System environment";
        m_dataPath = dataDirEnv;
#ifndef Q_OS_MACOS
        if (QFile::exists(FS::PathCombine(m_rootPath, "portable.txt"))) {
            m_isPortable = true;
        }
#endif
    } else {
        QDir foo(FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), ".."));
        m_dataPath = foo.absolutePath();
        adjustedBy = "Persistent data path";

#ifndef Q_OS_MACOS
        if (auto portableUserData = FS::PathCombine(m_rootPath, "UserData"); QDir(portableUserData).exists()) {
            m_dataPath = portableUserData;
            adjustedBy = "Portable user data path";
            m_isPortable = true;
        } else if (QFile::exists(FS::PathCombine(m_rootPath, "portable.txt"))) {
            m_dataPath = m_rootPath;
            adjustedBy = "Portable data path";
            m_isPortable = true;
        }
#endif
    }

    m_updateLogPath = FS::PathCombine(m_dataPath, "logs", "prism_launcher_update.log");

    {  // setup logging
        FS::ensureFolderPathExists(FS::PathCombine(m_dataPath, "logs"));
        static const QString s_baseLogFile = BuildConfig.LAUNCHER_NAME + "Updater" + (m_checkOnly ? "-CheckOnly" : "") + "-%0.log";
        static const QString s_logBase = FS::PathCombine(m_dataPath, "logs", s_baseLogFile);

        if (FS::ensureFolderPathExists("logs")) {  // enough history to track both launches of the updater during a portable install
            FS::move(s_logBase.arg(1), s_logBase.arg(2));
            FS::move(s_logBase.arg(0), s_logBase.arg(1));
        }

        logFile = std::make_unique<QFile>(s_logBase.arg(0));
        if (!logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            showFatalErrorMessage(tr("The launcher data folder is not writable!"),
                                  tr("The updater couldn't create a log file - %1.\n"
                                     "\n"
                                     "Make sure you have write permissions to the data folder.\n"
                                     "(%2)\n"
                                     "\n"
                                     "The updater cannot continue until you fix this problem.")
                                      .arg(logFile->errorString())
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
            QStringList ruleNames = loggingRules.childKeys();
            QStringList rules;
            qDebug() << "Setting log rules:";
            for (const auto& ruleName : ruleNames) {
                auto rule = QString("%1=%2").arg(ruleName).arg(loggingRules.value(ruleName).toString());
                rules.append(rule);
                qDebug() << "    " << rule;
            }
            auto rulesStr = rules.join("\n");
            QLoggingCategory::setFilterRules(rulesStr);
        }

        qDebug() << "<> Log initialized.";
    }

    {  // log debug program info
        qDebug() << qPrintable(BuildConfig.LAUNCHER_DISPLAYNAME + " Updater, " +
                               QString(BuildConfig.LAUNCHER_COPYRIGHT).replace("\n", ", "));
        qDebug() << "Version                    :" << BuildConfig.printableVersionString();
        qDebug() << "Git commit                 :" << BuildConfig.GIT_COMMIT;
        qDebug() << "Git refspec                :" << BuildConfig.GIT_REFSPEC;
        qDebug() << "Compiled for               :" << BuildConfig.systemID();
        qDebug() << "Compiled by                :" << BuildConfig.compilerID();
        qDebug() << "Build Artifact             :" << BuildConfig.BUILD_ARTIFACT;
        if (!adjustedBy.isEmpty()) {
            qDebug() << "Data dir before adjustment :" << origCwdPath;
            qDebug() << "Data dir after adjustment  :" << m_dataPath;
            qDebug() << "Adjusted by                :" << adjustedBy;
        } else {
            qDebug() << "Data dir                   :" << QDir::currentPath();
        }
        qDebug() << "Work dir                   :" << QDir::currentPath();
        qDebug() << "Binary path                :" << binPath;
        qDebug() << "Application root path      :" << m_rootPath;
        qDebug() << "Portable install           :" << m_isPortable;
        qDebug() << "<> Paths set.";
    }

    {  // network
        m_network = std::make_unique<QNetworkAccessManager>();
        qDebug() << "Detecting proxy settings...";
        QNetworkProxy proxy = QNetworkProxy::applicationProxy();
        m_network->setProxy(proxy);
    }

#ifdef Q_OS_MACOS
    showFatalErrorMessage(tr("MacOS Not Supported"), tr("The updater does not support installations on MacOS"));
#endif

    if (binPath.startsWith("/tmp/.mount_")) {
        m_isAppimage = true;
        m_appimagePath = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));
        if (m_appimagePath.isEmpty()) {
            showFatalErrorMessage(tr("Unsupported Installation"),
                                  tr("Updater is running as misconfigured AppImage? ($APPIMAGE environment variable is missing)"));
        }
    }

    m_isFlatpak = DesktopServices::isFlatpak();

    QString prismExecutable = FS::PathCombine(binPath, BuildConfig.LAUNCHER_APP_BINARY_NAME);
#if defined Q_OS_WIN32
    prismExecutable.append(".exe");
#endif

    if (!QFileInfo(prismExecutable).isFile()) {
        showFatalErrorMessage(tr("Unsupported Installation"), tr("The updater can not find the main executable."));
    }

    m_prismExecutable = prismExecutable;

    auto prismUpdateUrl = parser.value("update-url");
    if (prismUpdateUrl.isEmpty()) {
        prismUpdateUrl = BuildConfig.UPDATER_GITHUB_REPO;
    }

    m_prismRepoUrl = QUrl::fromUserInput(prismUpdateUrl);

    m_checkOnly = parser.isSet("check-only");
    m_forceUpdate = parser.isSet("force");
    m_printOnly = parser.isSet("list");
    auto userVersion = parser.value("install-version");
    if (!userVersion.isEmpty()) {
        m_userSelectedVersion = Version(userVersion);
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
        auto versionParts = version.split('.');
        m_prismVersionMajor = versionParts.takeFirst().toInt();
        m_prismVersionMinor = versionParts.takeFirst().toInt();
        if (!versionParts.isEmpty()) {
            m_prismVersionPatch = versionParts.takeFirst().toInt();
        } else {
            m_prismVersionPatch = 0;
        }
    }

    m_allowPreRelease = parser.isSet("pre-release");

    auto markerFilePath = QDir(m_rootPath).absoluteFilePath(".prism_launcher_updater_unpack.marker");
    auto markerFile = QFileInfo(markerFilePath);
    if (markerFile.exists()) {
        auto targetDir = QString(FS::read(markerFilePath)).trimmed();
        if (targetDir.isEmpty()) {
            qWarning() << "Empty updater marker file contains no install target. making best guess of parent dir";
            targetDir = QDir(m_rootPath).absoluteFilePath("..");
        }

        QMetaObject::invokeMethod(this, [this, targetDir]() { moveAndFinishUpdate(targetDir); }, Qt::QueuedConnection);

    } else {
        QMetaObject::invokeMethod(this, &PrismUpdaterApp::loadReleaseList, Qt::QueuedConnection);
    }
}

PrismUpdaterApp::~PrismUpdaterApp()
{
    qDebug() << "updater shutting down";
    // Shut down logger by setting the logger function to nothing
    qInstallMessageHandler(nullptr);
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
    auto* msgBox = new QMessageBox();
    msgBox->setWindowTitle(title);
    msgBox->setText(content);
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setDefaultButton(QMessageBox::Ok);
    msgBox->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
    msgBox->setIcon(QMessageBox::Critical);
    msgBox->setMinimumWidth(460);
    msgBox->adjustSize();
    msgBox->exec();
    exit(1);
}

void PrismUpdaterApp::run()
{
    qDebug() << "found" << m_releases.length() << "releases on github";
    qDebug() << "loading exe at" << m_prismExecutable;

    if (m_printOnly) {
        printReleases();
        m_status = Succeeded;
        exit(0);
        return;
    }

    if (!loadPrismVersionFromExe(m_prismExecutable)) {
        m_prismVersion = BuildConfig.printableVersionString();
        m_prismVersionMajor = BuildConfig.VERSION_MAJOR;
        m_prismVersionMinor = BuildConfig.VERSION_MINOR;
        m_prismVersionPatch = BuildConfig.VERSION_PATCH;
        m_prsimVersionChannel = BuildConfig.VERSION_CHANNEL;
        m_prismGitCommit = BuildConfig.GIT_COMMIT;
    }
    m_status = Succeeded;

    qDebug() << "Executable reports as:" << m_prismBinaryName << "version:" << m_prismVersion;
    qDebug() << "Version major:" << m_prismVersionMajor;
    qDebug() << "Version minor:" << m_prismVersionMinor;
    qDebug() << "Version minor:" << m_prismVersionPatch;
    qDebug() << "Version channel:" << m_prsimVersionChannel;
    qDebug() << "Git Commit:" << m_prismGitCommit;

    auto latest = getLatestRelease();
    qDebug() << "Latest release" << latest.version;
    auto needUpdateVal = needUpdate(latest);

    if (m_checkOnly) {
        if (needUpdateVal) {
            QTextStream stdOutStream(stdout);
            stdOutStream << "Name: " << latest.name << "\n";
            stdOutStream << "Version: " << latest.tag_name << "\n";
            stdOutStream << "TimeStamp: " << latest.created_at.toString(Qt::ISODate) << "\n";
            stdOutStream << latest.body << "\n";
            stdOutStream.flush();

            exit(100);
            return;
        }
        exit(0);
        return;
    }

    if (m_isFlatpak) {
        showFatalErrorMessage(tr("Updating flatpack not supported"), tr("Actions outside of checking if an update is available are not "
                                                                        "supported when running the flatpak version of Prism Launcher."));
        return;
    }
    if (m_isAppimage) {
        bool result = true;
        if (needUpdateVal) {
            result = callAppImageUpdate();
        }
        exit(result ? 0 : 1);
        return;
    }

    if (needUpdateVal || m_forceUpdate || !m_userSelectedVersion.isEmpty()) {
        GitHubRelease updateRelease = latest;
        if (!m_userSelectedVersion.isEmpty()) {
            bool found = false;
            for (const auto& rls : m_releases) {
                if (rls.version == m_userSelectedVersion) {
                    found = true;
                    updateRelease = rls;
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
            updateRelease = selectRelease();
            if (!updateRelease.isValid()) {
                showFatalErrorMessage("No version selected.", "No version was selected.");
                return;
            }
        }

        performUpdate(updateRelease);
    }

    exit(0);
}

void PrismUpdaterApp::moveAndFinishUpdate(const QDir& target)
{
    logUpdate("Finishing update process");

    logUpdate("Waiting 2 seconds for resources to free");
    QThread::sleep(2);

    auto manifestPath = FS::PathCombine(m_rootPath, "manifest.txt");
    QFileInfo manifest(manifestPath);

    auto appDir = QDir(m_rootPath);

    QStringList fileList;
    if (manifest.isFile()) {
        // load manifest from file
        logUpdate(tr("Reading manifest from %1").arg(manifest.absoluteFilePath()));
        try {
            auto contents = QString::fromUtf8(FS::read(manifest.absoluteFilePath()));
            auto files = contents.split('\n');
            for (const auto& file : files) {
                fileList.append(file.trimmed());
            }
        } catch (FS::FileSystemException& err) {
            qWarning() << "Failed to read manifest:" << err.what() << "\n" << err.cause();
        }
    }

    if (fileList.isEmpty()) {
        logUpdate(tr("Manifest empty, making best guess of the directory contents of %1").arg(m_rootPath));
        auto entries = target.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
        for (const auto& entry : entries) {
            fileList.append(entry.fileName());
        }
    }
    logUpdate(tr("Installing the following to %1 :\n %2").arg(target.absolutePath()).arg(fileList.join(",\n  ")));

    bool error = false;

    QProgressDialog progress(tr("Installing from %1").arg(m_rootPath), "", 0, static_cast<int>(fileList.length()));
    progress.setCancelButton(nullptr);
    progress.setMinimumWidth(400);
    progress.adjustSize();
    progress.show();
    QCoreApplication::processEvents();

    logUpdate(tr("Installing from %1").arg(m_rootPath));

    auto copy = [this, appDir, target](const QString& toInstallFile) {
        auto relPath = appDir.relativeFilePath(toInstallFile);
        auto installPath = FS::PathCombine(target.absolutePath(), relPath);
        logUpdate(tr("Installing %1 from %2").arg(installPath).arg(toInstallFile));
        FS::ensureFilePathExists(installPath);
        auto result = FS::copy(toInstallFile, installPath).overwrite(true)();
        if (!result) {
            logUpdate(tr("Failed copy %1 to %2").arg(toInstallFile).arg(installPath));
            return true;
        }
        return false;
    };

    int i = 0;
    for (const auto& glob : fileList) {
        progress.setValue(i);
        QCoreApplication::processEvents();
        QList<QString> matches;
        if (!glob.isEmpty()) {
            for (const auto& entry : QDirListing(m_rootPath, { glob }, QDirListing::IteratorFlag::ResolveSymlinks)) {
                matches.append(entry.absoluteFilePath());
            }
        }
        if (matches.isEmpty() && !glob.isEmpty()) {
            if (auto fileInfo = QFileInfo(FS::PathCombine(m_rootPath, glob)); fileInfo.exists()) {
                error |= copy(fileInfo.absoluteFilePath());
            } else {
                logUpdate(tr("File doesn't exist, ignoring: %1").arg(FS::PathCombine(m_rootPath, glob)));
            }
        } else {
            for (const auto& path : matches) {
                error |= copy(path);
            }
        }
        i++;
    }
    progress.setValue(i);
    QCoreApplication::processEvents();

    if (error) {
        logUpdate(tr("There were errors installing the update."));
        auto failMarker = FS::PathCombine(m_dataPath, ".prism_launcher_update.fail");
        FS::copy(m_updateLogPath, failMarker).overwrite(true)();
    } else {
        logUpdate(tr("Update succeed."));
        auto successMarker = FS::PathCombine(m_dataPath, ".prism_launcher_update.success");
        FS::copy(m_updateLogPath, successMarker).overwrite(true)();
    }
    auto updateLockPath = FS::PathCombine(m_dataPath, ".prism_launcher_update.lock");
    FS::deletePath(updateLockPath);

    QProcess proc;
    auto appExeName = BuildConfig.LAUNCHER_APP_BINARY_NAME;
#if defined Q_OS_WIN32
    appExeName.append(".exe");

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("__COMPAT_LAYER", "RUNASINVOKER");
    proc.setProcessEnvironment(env);
#else
    appExeName.prepend("bin/");
#endif

    auto appExePath = target.absoluteFilePath(appExeName);
    proc.setProgram(appExePath);
    proc.startDetached();

    exit(error ? 1 : 0);
}

void PrismUpdaterApp::printReleases()
{
    for (const auto& release : m_releases) {
        std::cout << release.name.toStdString() << " Version: " << release.tag_name.toStdString() << '\n';
    }
}

QList<GitHubRelease> PrismUpdaterApp::nonDraftReleases()
{
    QList<GitHubRelease> nonDraft;
    for (const auto& rls : m_releases) {
        if (rls.isValid() && !rls.draft) {
            nonDraft.append(rls);
        }
    }
    return nonDraft;
}

QList<GitHubRelease> PrismUpdaterApp::newerReleases()
{
    QList<GitHubRelease> newer;
    for (const auto& rls : nonDraftReleases()) {
        if (rls.version > m_prismVersion) {
            newer.append(rls);
        }
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

    if (releases.isEmpty()) {
        return {};
    }

    SelectReleaseDialog dlg(Version(m_prismVersion), releases);
    auto result = dlg.exec();

    if (result == QDialog::Rejected) {
        return {};
    }
    GitHubRelease release = dlg.selectedRelease();

    return release;
}

QList<GitHubReleaseAsset> PrismUpdaterApp::validReleaseArtifacts(const GitHubRelease& release) const
{
    QList<GitHubReleaseAsset> valid;

    qDebug() << "Selecting best asset from" << release.tag_name << "for platform" << BuildConfig.BUILD_ARTIFACT
             << "portable:" << m_isPortable;
    if (BuildConfig.BUILD_ARTIFACT.isEmpty()) {
        qWarning() << "Build platform is not set!";
    }
    for (const auto& asset : release.assets) {
        if (asset.name.endsWith(".zsync")) {
            qDebug() << "Rejecting zsync file" << asset.name;
            continue;
        }
        if (!m_isAppimage && asset.name.toLower().endsWith("appimage")) {
            qDebug() << "Rejecting" << asset.name << "because it is an AppImage";
            continue;
        }
        if (m_isAppimage && !asset.name.toLower().endsWith("appimage")) {
            qDebug() << "Rejecting" << asset.name << "because it is not an AppImage";
            continue;
        }
        auto assetName = asset.name.toLower();
        auto [platform, platformQtVer] = StringUtils::splitFirst(BuildConfig.BUILD_ARTIFACT.toLower(), "-qt");
        auto systemIsArm = QSysInfo::buildCpuArchitecture().contains("arm64");
        auto assetIsArm = assetName.contains("arm64");
        auto assetIsArchive = assetName.endsWith(".zip") || assetName.endsWith(".tar.gz");

        bool forPlatform = !platform.isEmpty() && assetName.contains(platform);
        if (!forPlatform) {
            qDebug() << "Rejecting" << asset.name << "because platforms do not match";
        }
        bool forPortable = assetName.contains("portable");
        if (forPlatform && assetName.contains("legacy") && !platform.contains("legacy")) {
            qDebug() << "Rejecting" << asset.name << "because platforms do not match";
            forPlatform = false;
        }
        if (forPlatform && ((assetIsArm && !systemIsArm) || (!assetIsArm && systemIsArm))) {
            qDebug() << "Rejecting" << asset.name << "because architecture does not match";
            forPlatform = false;
        }
        if (forPlatform && platform.contains("windows") && !m_isPortable && assetIsArchive) {
            qDebug() << "Rejecting" << asset.name << "because it is not an installer";
            forPlatform = false;
        }

        static const QRegularExpression s_qtPattern("-qt(\\d+)");
        auto qtMatch = s_qtPattern.match(assetName);
        if (forPlatform && qtMatch.hasMatch()) {
            if (platformQtVer.isEmpty() || platformQtVer.toInt() != qtMatch.captured(1).toInt()) {
                qDebug() << "Rejecting" << asset.name << "because it is not for the correct qt version" << platformQtVer.toInt() << "vs"
                         << qtMatch.captured(1).toInt();
                forPlatform = false;
            }
        }

        if (((m_isPortable && forPortable) || (!m_isPortable && !forPortable)) && forPlatform) {
            qDebug() << "Accepting" << asset.name;
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
    m_installRelease = release;
    qDebug() << "Updating to" << release.tag_name;
    auto validAssets = validReleaseArtifacts(release);
    qDebug() << "valid release assets:" << validAssets;

    GitHubReleaseAsset selectedAsset;
    if (validAssets.isEmpty()) {
        showFatalErrorMessage(tr("No Valid Release Assets"),
                              tr("Github release %1 has no valid assets for this platform: %2")
                                  .arg(release.tag_name)
                                  .arg(tr("%1 portable: %2").arg(BuildConfig.BUILD_ARTIFACT).arg(m_isPortable ? tr("yes") : tr("no"))));
        return;
    }
    if (validAssets.length() > 1) {
        selectedAsset = selectAsset(validAssets);
    } else {
        selectedAsset = validAssets.takeFirst();
    }

    if (!selectedAsset.isValid()) {
        showFatalErrorMessage(tr("No version selected."), tr("No version was selected."));
        return;
    }

    qDebug() << "will install" << selectedAsset;
    auto file = downloadAsset(selectedAsset);

    if (!file.exists()) {
        showFatalErrorMessage(tr("Failed to Download"), tr("Failed to download the selected asset."));
        return;
    }

    performInstall(file);
}

QFileInfo PrismUpdaterApp::downloadAsset(const GitHubReleaseAsset& asset)
{
    auto tempDir = QDir::tempPath();
    auto fileUrl = QUrl(asset.browser_download_url);
    auto outFilePath = FS::PathCombine(tempDir, fileUrl.fileName());

    qDebug() << "downloading" << fileUrl << "to" << outFilePath;
    auto download = Net::Request::makeFile(fileUrl, outFilePath);
    download->setNetwork(m_network.get());
    auto progressDialog = ProgressDialog();
    progressDialog.adjustSize();

    progressDialog.execWithTask(download.get());

    qDebug() << "download complete";

    QFileInfo outFile(outFilePath);
    return outFile;
}

bool PrismUpdaterApp::callAppImageUpdate()
{
    auto appimagePath = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));
    QProcess proc = QProcess();
    qDebug() << "Calling: AppImageUpdate" << appimagePath;
    proc.setProgram(FS::PathCombine(m_rootPath, "bin", "AppImageUpdate.AppImage"));
    proc.setArguments({ appimagePath });
    auto result = proc.startDetached();
    if (!result) {
        qDebug() << "Failed to start AppImageUpdate reason:" << proc.errorString();
    }
    return result;
}

void PrismUpdaterApp::clearUpdateLog()
{
    FS::deletePath(m_updateLogPath);
}

void PrismUpdaterApp::logUpdate(const QString& msg)
{
    qDebug() << qUtf8Printable(msg);
    FS::append(m_updateLogPath, QStringLiteral("%1\n").arg(msg).toUtf8());
}

namespace {

std::tuple<QDateTime, QString, QString, QString, QString> readLockFile(const QString& path)
{
    auto contents = QString(FS::read(path));
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

bool writeLockFile(const QString& path,
                   const QDateTime& timestamp,
                   const QString& from,
                   const QString& to,
                   const QString& target,
                   const QString& dataPath)
{
    try {
        FS::write(path, QStringLiteral("TIMESTAMP=%1\nFROM=%2\nTO=%3\nTARGET=%4\nDATA_PATH=%5\n")
                            .arg(timestamp.toString(Qt::ISODate))
                            .arg(from)
                            .arg(to)
                            .arg(target)
                            .arg(dataPath)
                            .toUtf8());
    } catch (FS::FileSystemException& err) {
        qWarning() << "Error writing lockfile:" << err.what() << "\n" << err.cause();
        return false;
    }
    return true;
}

}  // namespace

void PrismUpdaterApp::performInstall(const QFileInfo& file)
{
    qDebug() << "starting install";
    auto updateLockPath = FS::PathCombine(m_dataPath, ".prism_launcher_update.lock");
    QFileInfo updateLock(updateLockPath);
    if (updateLock.exists()) {
        auto [timestamp, from, to, target, dataPath] = readLockFile(updateLockPath);
        auto msg = tr("Update already in progress\n");
        auto infoMsg =
            tr("This installation has a update lock file present at: %1\n"
               "\n"
               "Timestamp: %2\n"
               "Updating from version %3 to %4\n"
               "Target install path: %5\n"
               "Data Path: %6"
               "\n"
               "This likely means that a previous update attempt failed. Please ensure your installation is in working order before "
               "proceeding.\n"
               "Check the Prism Launcher updater log at: \n"
               "%7\n"
               "for details on the last update attempt.\n"
               "\n"
               "To overwrite this lock and proceed with this update anyway, select \"Ignore\" below.")
                .arg(updateLockPath)
                .arg(timestamp.toString(Qt::ISODate), from, to, target, dataPath)
                .arg(m_updateLogPath);
        QMessageBox msgBox;
        msgBox.setText(msg);
        msgBox.setInformativeText(infoMsg);
        msgBox.setStandardButtons(QMessageBox::Ignore | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setMinimumWidth(460);
        msgBox.adjustSize();
        switch (msgBox.exec()) {
            case QMessageBox::AcceptRole:
                break;
            case QMessageBox::RejectRole:
                [[fallthrough]];
            default: {
                showFatalErrorMessage(tr("Update Aborted"), tr("The update attempt was aborted"));
                return;
            }
        }
    }
    clearUpdateLog();

    auto changelogPath = FS::PathCombine(m_dataPath, ".prism_launcher_update.changelog");
    FS::write(changelogPath, m_installRelease.body.toUtf8());

    logUpdate(tr("Updating from %1 to %2").arg(m_prismVersion).arg(m_installRelease.tag_name));
    if (m_isPortable || file.fileName().endsWith(".zip") || file.fileName().endsWith(".tar.gz")) {
        writeLockFile(updateLockPath, QDateTime::currentDateTime(), m_prismVersion, m_installRelease.tag_name, m_rootPath, m_dataPath);
        logUpdate(tr("Updating portable install at %1").arg(m_rootPath));
        unpackAndInstall(file);
    } else {
        logUpdate(tr("Running installer file at %1").arg(file.absoluteFilePath()));
        QProcess proc = QProcess();
#if defined Q_OS_WIN
        auto env = QProcessEnvironment::systemEnvironment();
        env.insert("__COMPAT_LAYER", "RUNASINVOKER");
        proc.setProcessEnvironment(env);
#endif
        proc.setProgram(file.absoluteFilePath());
        bool result = proc.startDetached();
        logUpdate(tr("Process start result: %1").arg(result ? tr("yes") : tr("no")));
        exit(result ? 0 : 1);
    }
}

void PrismUpdaterApp::unpackAndInstall(const QFileInfo& archive)
{
    logUpdate(tr("Backing up install"));
    backupAppDir();

    if (auto loc = unpackArchive(archive)) {
        auto markerFilePath = loc.value().absoluteFilePath(".prism_launcher_updater_unpack.marker");
        FS::write(markerFilePath, m_rootPath.toUtf8());

        QProcess proc = QProcess();

        auto exeName = QStringLiteral("%1_updater").arg(BuildConfig.LAUNCHER_APP_BINARY_NAME);
#if defined Q_OS_WIN32
        exeName.append(".exe");

        auto env = QProcessEnvironment::systemEnvironment();
        env.insert("__COMPAT_LAYER", "RUNASINVOKER");
        proc.setProcessEnvironment(env);
#else
        exeName.prepend("bin/");
#endif

        auto newUpdaterPath = loc.value().absoluteFilePath(exeName);
        logUpdate(tr("Starting new updater at '%1'").arg(newUpdaterPath));
        proc.setProgram(newUpdaterPath);
        proc.setArguments({ "-d", m_dataPath });
        proc.setWorkingDirectory(loc.value().absolutePath());
        if (!proc.startDetached()) {
            logUpdate(tr("Failed to launch '%1' %2").arg(newUpdaterPath).arg(proc.errorString()));
            exit(10);
            return;
        }
        exit();
        return;  // up to the new updater now
    }
    exit(1);  // unpack failure
}

void PrismUpdaterApp::backupAppDir()
{
    auto manifestPath = FS::PathCombine(m_rootPath, "manifest.txt");
    QFileInfo manifest(manifestPath);

    QStringList fileList;
    if (manifest.isFile()) {
        // load manifest from file

        logUpdate(tr("Reading manifest from %1").arg(manifest.absoluteFilePath()));
        try {
            auto contents = QString::fromUtf8(FS::read(manifest.absoluteFilePath()));
            auto files = contents.split('\n');
            for (const auto& file : files) {
                fileList.append(file.trimmed());
            }
        } catch (FS::FileSystemException& err) {
            qWarning() << "Failed to read manifest during backup:" << err.what() << "\n" << err.cause();
        }
    }

    if (fileList.isEmpty()) {
        // best guess
        if (BuildConfig.BUILD_ARTIFACT.toLower().contains("linux")) {
            fileList.append({ "PrismLauncher", "bin", "share", "lib" });
        } else {  // windows by process of elimination
            fileList.append({
                "jars",
                "prismlauncher.exe",
                "prismlauncher_filelink.exe",
                "prismlauncher_updater.exe",
                "qtlogging.ini",
                "imageformats",
                "iconengines",
                "platforms",
                "styles",
                "tls",
                "qt.conf",
                "Qt*.dll",
            });
        }
        logUpdate("manifest.txt empty or missing. making best guess at files to back up.");
    }
    logUpdate(tr("Backing up:\n  %1").arg(fileList.join(",\n  ")));
    static const QRegularExpression s_replaceRegex("[" + QRegularExpression::escape("\\/:*?\"<>|") + "]");
    auto appDir = QDir(m_rootPath);
    auto backupDir =
        FS::PathCombine(appDir.absolutePath(),
                        QStringLiteral("backup_") + QString(m_prismVersion).replace(s_replaceRegex, QString("_")) + "-" + m_prismGitCommit);
    FS::ensureFolderPathExists(backupDir);
    auto backupMarkerPath = FS::PathCombine(m_dataPath, ".prism_launcher_update_backup_path.txt");
    FS::write(backupMarkerPath, backupDir.toUtf8());

    QProgressDialog progress(tr("Backing up install at %1").arg(m_rootPath), "", 0, static_cast<int>(fileList.length()));
    progress.setCancelButton(nullptr);
    progress.setMinimumWidth(400);
    progress.adjustSize();
    progress.show();
    QCoreApplication::processEvents();

    logUpdate(tr("Backing up install at %1").arg(m_rootPath));

    auto copy = [this, appDir, backupDir](const QString& toBakFile) {
        auto relPath = appDir.relativeFilePath(toBakFile);
        auto bakPath = FS::PathCombine(backupDir, relPath);
        logUpdate(tr("Backing up and then removing %1").arg(toBakFile));
        FS::ensureFilePathExists(bakPath);
        auto result = FS::copy(toBakFile, bakPath).overwrite(true)();
        if (!result) {
            logUpdate(tr("Failed to backup %1 to %2").arg(toBakFile).arg(bakPath));
        } else {
            if (!FS::deletePath(toBakFile)) {
                logUpdate(tr("Failed to remove %1").arg(toBakFile));
            }
        }
    };

    int i = 0;
    for (const auto& glob : fileList) {
        progress.setValue(i);
        QCoreApplication::processEvents();
        QList<QString> matches;
        if (!glob.isEmpty()) {
            for (const auto& entry : QDirListing(appDir.absolutePath(), { glob }, QDirListing::IteratorFlag::ResolveSymlinks)) {
                matches.append(entry.absoluteFilePath());
            }
        }
        if (matches.isEmpty() && !glob.isEmpty()) {
            if (auto fileInfo = QFileInfo(FS::PathCombine(appDir.absolutePath(), glob)); fileInfo.exists()) {
                copy(fileInfo.absoluteFilePath());
            } else {
                logUpdate(tr("File doesn't exist, ignoring: %1").arg(FS::PathCombine(appDir.absolutePath(), glob)));
            }
        } else {
            for (const auto& path : matches) {
                copy(path);
            }
        }
        i++;
    }
    progress.setValue(i);
    QCoreApplication::processEvents();
}

std::optional<QDir> PrismUpdaterApp::unpackArchive(const QFileInfo& archive)
{
    auto tempExtractPath = FS::PathCombine(m_dataPath, "prism_launcher_update_release");
    FS::ensureFolderPathExists(tempExtractPath);
    auto tmpExtractDir = QDir(tempExtractPath);

    auto result = MMCZip::extractDir(archive.absoluteFilePath(), tmpExtractDir.absolutePath());
    if (result) {
        logUpdate(tr("Extracted the following to \"%1\":\n  %2").arg(tmpExtractDir.absolutePath()).arg(result->join("\n  ")));
    } else {
        logUpdate(tr("Failed to extract %1 to %2").arg(archive.absoluteFilePath()).arg(tmpExtractDir.absolutePath()));
        showFatalErrorMessage("Failed to extract archive",
                              tr("Failed to extract %1 to %2").arg(archive.absoluteFilePath()).arg(tmpExtractDir.absolutePath()));
        return std::nullopt;
    }

    return tmpExtractDir;
}

bool PrismUpdaterApp::loadPrismVersionFromExe(const QString& exePath)
{
    QProcess proc = QProcess();
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.setReadChannel(QProcess::StandardOutput);
    proc.start(exePath, { "--version" });
    if (!proc.waitForStarted(5000)) {
        showFatalErrorMessage(tr("Failed to Check Version"), tr("Failed to launch child process to read version."));
        return false;
    }  // wait 5 seconds to start
    if (!proc.waitForFinished(5000)) {
        showFatalErrorMessage(tr("Failed to Check Version"), tr("Child launcher process failed."));
        return false;
    }
    auto out = proc.readAllStandardOutput();
    auto lines = out.split('\n');
    lines.removeAll("");
    if (lines.length() < 2) {
        return false;
    }
    if (lines.length() > 2) {
        auto line1 = lines.takeLast();
        auto line2 = lines.takeLast();
        lines = { line2, line1 };
    }
    auto first = lines.takeFirst();
    auto firstParts = first.split(' ');
    if (firstParts.length() < 2) {
        return false;
    }
    m_prismBinaryName = firstParts.takeFirst();
    auto version = firstParts.takeFirst().trimmed();
    m_prismVersion = version;
    if (version.contains('-')) {
        auto index = version.indexOf('-');
        m_prsimVersionChannel = version.mid(index + 1);
        version = version.left(index);
    } else {
        m_prsimVersionChannel = "stable";
    }
    auto versionParts = version.split('.');
    if (versionParts.length() < 2) {
        return false;
    }
    m_prismVersionMajor = versionParts.takeFirst().toInt();
    m_prismVersionMinor = versionParts.takeFirst().toInt();
    if (!versionParts.isEmpty()) {
        m_prismVersionPatch = versionParts.takeFirst().toInt();
    } else {
        m_prismVersionPatch = 0;
    }
    m_prismGitCommit = lines.takeFirst().simplified();
    return true;
}

void PrismUpdaterApp::loadReleaseList()
{
    auto githubRepo = m_prismRepoUrl;
    if (githubRepo.host() != "github.com") {
        fail("updating from a non github url is not supported");
        return;
    }

    auto pathParts = githubRepo.path().split('/');
    pathParts.removeFirst();  // empty segment from leading /
    auto repoOwner = pathParts.takeFirst();
    auto repoName = pathParts.takeFirst();
    auto apiUrl = QString("https://api.github.com/repos/%1/%2/releases").arg(repoOwner, repoName);

    qDebug() << "Fetching release list from" << apiUrl;

    downloadReleasePage(apiUrl, 1);
}

void PrismUpdaterApp::downloadReleasePage(const QString& apiUrl, int page)
{
    int perPage = 30;
    auto pageUrl = QString("%1?per_page=%2&page=%3").arg(apiUrl).arg(QString::number(perPage)).arg(QString::number(page));
    auto [download, response] = Net::Request::makeByteArray(pageUrl);
    download->setNetwork(m_network.get());
    m_currentUrl = pageUrl;

    auto githubApiHeaders = std::make_unique<Net::RawHeaderProxy>();
    githubApiHeaders->addHeaders({
        { .headerName = "Accept", .headerValue = "application/vnd.github+json" },
        { .headerName = "X-GitHub-Api-Version", .headerValue = "2022-11-28" },
    });
    download->addHeaderProxy(std::move(githubApiHeaders));

    connect(download.get(), &Net::Request::succeeded, this, [this, response, perPage, apiUrl, page]() {
        int numFound = parseReleasePage(response);
        if (!(numFound < perPage)) {  // there may be more, fetch next page
            downloadReleasePage(apiUrl, page + 1);
        } else {
            run();
        }
    });
    connect(download.get(), &Net::Request::failed, this, &PrismUpdaterApp::downloadError);

    m_currentTask.reset(download);
    connect(download.get(), &Net::Request::finished, this,
            [this]() { qDebug() << "Download" << m_currentTask->getUid().toString() << "finished"; });

    QCoreApplication::processEvents();

    QMetaObject::invokeMethod(download.get(), &Task::start, Qt::QueuedConnection);
}

int PrismUpdaterApp::parseReleasePage(const QByteArray* response)
{
    if (response->isEmpty()) {  // empty page
        return 0;
    }
    int numReleases = 0;
    try {
        auto doc = Json::requireDocument(*response);
        auto releaseList = Json::requireArray(doc);
        for (auto releaseJson : releaseList) {
            auto releaseObj = Json::requireObject(releaseJson);

            GitHubRelease release = {};
            release.id = Json::requireInteger(releaseObj, "id");
            release.name = releaseObj["name"].toString();
            release.tag_name = Json::requireString(releaseObj, "tag_name");
            release.created_at = QDateTime::fromString(Json::requireString(releaseObj, "created_at"), Qt::ISODate);
            release.published_at = QDateTime::fromString(releaseObj["published_at"].toString(), Qt::ISODate);
            release.draft = Json::requireBoolean(releaseObj, "draft");
            release.prerelease = Json::requireBoolean(releaseObj, "prerelease");
            release.body = releaseObj["body"].toString();
            release.version = Version(release.tag_name);

            auto releaseAssetsObj = Json::requireArray(releaseObj, "assets");
            for (auto assetJson : releaseAssetsObj) {
                auto assetObj = Json::requireObject(assetJson);
                GitHubReleaseAsset asset = {};
                asset.id = Json::requireInteger(assetObj, "id");
                asset.name = Json::requireString(assetObj, "name");
                asset.label = assetObj["label"].toString();
                asset.content_type = Json::requireString(assetObj, "content_type");
                asset.size = Json::requireInteger(assetObj, "size");
                asset.created_at = QDateTime::fromString(Json::requireString(assetObj, "created_at"), Qt::ISODate);
                asset.updated_at = QDateTime::fromString(Json::requireString(assetObj, "updated_at"), Qt::ISODate);
                asset.browser_download_url = Json::requireString(assetObj, "browser_download_url");
                release.assets.append(asset);
            }
            m_releases.append(release);
            numReleases++;
        }
    } catch (Json::JsonException& e) {
        auto errMsg =
            QString("Failed to parse releases from github: %1\n%2").arg(e.what()).arg(QString::fromStdString(response->toStdString()));
        fail(errMsg);
    }
    return numReleases;
}

GitHubRelease PrismUpdaterApp::getLatestRelease()
{
    GitHubRelease latest;
    for (const auto& release : m_releases) {
        if (release.draft) {
            continue;
        }
        if (release.prerelease && !m_allowPreRelease) {
            continue;
        }
        if (!latest.isValid() || (release.version > latest.version)) {
            latest = release;
        }
    }
    return latest;
}

bool PrismUpdaterApp::needUpdate(const GitHubRelease& release) const
{
    auto currentVer = Version(QString("%1.%2.%3").arg(m_prismVersionMajor).arg(m_prismVersionMinor).arg(m_prismVersionPatch));
    return currentVer < release.version;
}

void PrismUpdaterApp::downloadError(const QString& reason)
{
    fail(QString("Network request Failed: %1 with reason %2").arg(m_currentUrl).arg(reason));
}
