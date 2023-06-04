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
#include <stdio.h>
#include <windows.h>
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

PrismUpdaterApp::PrismUpdaterApp(int& argc, char** argv) : QApplication(argc, argv)
{
#if defined Q_OS_WIN32
    // attach the parent console
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // if attach succeeds, reopen and sync all the i/o
        if (freopen("CON", "w", stdout)) {
            std::cout.sync_with_stdio();
        }
        if (freopen("CON", "w", stderr)) {
            std::cerr.sync_with_stdio();
        }
        if (freopen("CON", "r", stdin)) {
            std::cin.sync_with_stdio();
        }
        auto out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD written;
        const char* endline = "\n";
        WriteConsole(out, endline, strlen(endline), &written, NULL);
        consoleAttached = true;
    }
#endif
    setOrganizationName(BuildConfig.LAUNCHER_NAME);
    setOrganizationDomain(BuildConfig.LAUNCHER_DOMAIN);
    setApplicationName(BuildConfig.LAUNCHER_NAME + "Updater");
    setApplicationVersion(BuildConfig.printableVersionString() + "\n" + BuildConfig.GIT_COMMIT);

    // Commandline parsing
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("An auto-updater for Prism Launcher"));

    parser.addOptions({ { { "d", "dir" }, tr("Use a custom path as application root (use '.' for current directory)."), tr("directory") },
                        { { "I", "install-version" }, "Install a spesfic version.", tr("version name") },
                        { { "U", "update-url" }, tr("Update from the spesified repo."), tr("github repo url") },
                        { { "c", "check-only" },
                          tr("Only check if an update is needed. Exit status 100 if true, 0 if false (or non 0 if there was an error).") },
                        { { "F", "force" }, tr("Force an update, even if one is not needed.") },
                        { { "l", "list" }, tr("List avalible releases.") },
                        { "debug", tr("Log debug to console.") },
                        { { "L", "latest" }, tr("Update to the latest avalible version.") },
                        { { "D", "allow-downgrade" }, tr("Allow the updater to downgrade to previous verisons.") } });

    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(arguments());

    logToConsole = parser.isSet("debug");

    auto prism_executable = QCoreApplication::applicationDirPath() + "/" + BuildConfig.LAUNCHER_APP_BINARY_NAME;
#if defined(Q_OS_WIN32)
    prism_executable += ".exe";
#endif

    if (BuildConfig.BUILD_PLATFORM.toLower() == "macos")
        showFatalErrorMessage(tr("MacOS Not Supported"), tr("The updater does not support instaltions on MacOS"));

    if (!QFileInfo(prism_executable).isFile())
        showFatalErrorMessage(tr("Unsupprted Instaltion"), tr("The updater can not find the main exacutable."));

    if (prism_executable.startsWith("/tmp/.mount_")) {
        m_appimage = true;
        m_appimagePath = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));
        if (m_appimagePath.isEmpty())
            showFatalErrorMessage(tr("Unsupprted Instaltion"),
                                  tr("Updater is running as misconfigured AppImage? ($APPIMAGE environment variable is missing)"));
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
    m_updateLatest = parser.isSet("latest");
    m_allowDowngrade = parser.isSet("allow-downgrade");

    QString origcwdPath = QDir::currentPath();
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
    QString dataPath;
    // change folder
    QString dirParam = parser.value("dir");
    if (!dirParam.isEmpty()) {
        // the dir param. it makes multimc data path point to whatever the user specified
        // on command line
        adjustedBy = "Command line";
        dataPath = dirParam;
    } else {
        QDir foo(FS::PathCombine(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), ".."));
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

    {  // setup logging
        static const QString logBase = BuildConfig.LAUNCHER_NAME + "Updater" + (m_checkOnly ? "-CheckOnly" : "") + "-%0.log";
        auto moveFile = [](const QString& oldName, const QString& newName) {
            QFile::remove(newName);
            QFile::copy(oldName, newName);
            QFile::remove(oldName);
        };

        moveFile(logBase.arg(3), logBase.arg(4));
        moveFile(logBase.arg(2), logBase.arg(3));
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
            qDebug() << "Work dir before adjustment : " << origcwdPath;
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
    qDebug() << "updater shutting down";
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

    loadPrismVersionFromExe(m_prismExecutable);
    m_status = Succeeded;

    qDebug() << "Executable reports as:" << m_prismBinaryName << "version:" << m_prismVerison;
    qDebug() << "Version major:" << m_prismVersionMajor;
    qDebug() << "Verison minor:" << m_prismVersionMinor;
    qDebug() << "Verison channel:" << m_prsimVersionChannel;
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
                    QString("Can not find a github relase for user spesified verison %1").arg(m_userSelectedVersion.toString()));
                return;
            }
        } else if (!m_updateLatest) {
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
        if (rls.version > m_prismVerison)
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

    SelectReleaseDialog dlg(Version(m_prismVerison), releases);
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

    qDebug() << "Selecting best asset from" << release.tag_name << "for platfom" << BuildConfig.BUILD_PLATFORM << "portable:" << m_portable;
    if (BuildConfig.BUILD_PLATFORM.isEmpty())
        qWarning() << "Build platform is not set!";
    for (auto asset : release.assets) {
        if (!m_appimage && asset.name.toLower().endsWith("appimage"))
            continue;
        else if (m_appimage && !asset.name.toLower().endsWith("appimage"))
            continue;

        bool for_platform = !BuildConfig.BUILD_PLATFORM.isEmpty() && asset.name.toLower().contains(BuildConfig.BUILD_PLATFORM.toLower());
        bool for_portable = asset.name.toLower().contains("portable");
        if (((m_portable && for_portable) || (!m_portable && !for_portable)) && for_platform) {
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
    qDebug() << "Updating to" << release.tag_name;
    auto valid_assets = validReleaseArtifacts(release);
    qDebug() << "vallid release assets:" << valid_assets;

    GitHubReleaseAsset selected_asset;
    if (valid_assets.isEmpty()) {
        showFatalErrorMessage(tr("No Valid Release Assets"),
                              tr("Github release %1 has no valid assets for this platform: %2")
                                  .arg(release.tag_name)
                                  .arg(tr("%1 portable: %2").arg(BuildConfig.BUILD_PLATFORM).arg(m_portable)));
        return;
    } else if (valid_assets.length() > 1) {
        selected_asset = selectAsset(valid_assets);
    } else {
        selected_asset = valid_assets.takeFirst();
    }

    if (! selected_asset.isValid()) {
        showFatalErrorMessage("No version selected.", "No version was selected.");
        return;
    }

    qDebug() << "will intall" << selected_asset;
}

QFileInfo PrismUpdaterApp::downloadAsset(const GitHubReleaseAsset& asset)
{
   // TODO! (researching what to do with appimages) 
}

void PrismUpdaterApp::loadPrismVersionFromExe(const QString& exe_path)
{
    QProcess proc = QProcess();
    proc.start(exe_path, { "-v" });
    proc.waitForFinished();
    auto out = proc.readAll();
    auto lines = out.split('\n');
    if (lines.length() < 2)
        return;
    auto first = lines.takeFirst();
    auto first_parts = first.split(' ');
    if (first_parts.length() < 2)
        return;
    m_prismBinaryName = first_parts.takeFirst();
    auto version = first_parts.takeFirst();
    m_prismVerison = version;
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
    auto responce = std::make_shared<QByteArray>();
    auto download = Net::Download::makeByteArray(page_url, responce.get());
    download->setNetwork(m_network);
    m_current_url = page_url;

    auto githup_api_headers = new Net::RawHeaderProxy();
    githup_api_headers->addHeaders({
        { "Accept", "application/vnd.github+json" },
        { "X-GitHub-Api-Version", "2022-11-28" },
    });
    download->addHeaderProxy(githup_api_headers);

    connect(download.get(), &Net::Download::succeeded, this, [this, responce, per_page, api_url, page]() {
        int num_found = parseReleasePage(responce.get());
        if (!(num_found < per_page)) {  // there may be more, fetch next page
            downloadReleasePage(api_url, page + 1);
        } else {
            run();
        }
    });
    connect(download.get(), &Net::Download::failed, this, &PrismUpdaterApp::downloadError);

    m_current_task.reset(download);
    connect(download.get(), &Net::Download::finished, this, [this]() {
        qDebug() << "Download" << m_current_task->getUid().toString() << "finsihed";
        m_current_task.reset();
        m_current_url = "";
    });

    QCoreApplication::processEvents();

    QMetaObject::invokeMethod(download.get(), &Task::start, Qt::QueuedConnection);
}

int PrismUpdaterApp::parseReleasePage(const QByteArray* responce)
{
    if (responce->isEmpty())  // empty page
        return 0;
    int num_releases = 0;
    try {
        auto doc = Json::requireDocument(*responce);
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
            QString("Failed to parse releases from github: %1\n%2").arg(e.what()).arg(QString::fromStdString(responce->toStdString()));
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
