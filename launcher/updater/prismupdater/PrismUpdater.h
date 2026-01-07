// SPDX-FileCopyrightText: 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
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
 */

#pragma once

#include <QtCore>

#include <QApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFlag>
#include <QIcon>
#include <QLocalSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <memory>
#include <optional>

#include "QObjectPtr.h"
#include "net/Download.h"

#define PRISM_EXTERNAL_EXE
#include "FileSystem.h"

#include "GitHubRelease.h"

class PrismUpdaterApp : public QApplication {
    // friends for the purpose of limiting access to deprecated stuff
    Q_OBJECT
   public:
    enum Status { Starting, Failed, Succeeded, Initialized, Aborted };
    PrismUpdaterApp(int& argc, char** argv);
    virtual ~PrismUpdaterApp();
    void loadReleaseList();
    void run();
    Status status() const { return m_status; }

   private:
    void fail(const QString& reason);
    void abort(const QString& reason);
    void showFatalErrorMessage(const QString& title, const QString& content);

    bool loadPrismVersionFromExe(const QString& exe_path);

    void downloadReleasePage(const QString& api_url, int page);
    int parseReleasePage(const QByteArray* response);

    bool needUpdate(const GitHubRelease& release);

    GitHubRelease getLatestRelease();
    GitHubRelease selectRelease();
    QList<GitHubRelease> newerReleases();
    QList<GitHubRelease> nonDraftReleases();

    void printReleases();

    QList<GitHubReleaseAsset> validReleaseArtifacts(const GitHubRelease& release);
    GitHubReleaseAsset selectAsset(const QList<GitHubReleaseAsset>& assets);
    void performUpdate(const GitHubRelease& release);
    void performInstall(QFileInfo file);
    void unpackAndInstall(QFileInfo file);
    void backupAppDir();
    std::optional<QDir> unpackArchive(QFileInfo file);

    QFileInfo downloadAsset(const GitHubReleaseAsset& asset);
    bool callAppImageUpdate();

    void moveAndFinishUpdate(QDir target);

   public slots:
    void downloadError(QString reason);

   private:
    const QString& root() { return m_rootPath; }

    bool isPortable() { return m_isPortable; }

    void clearUpdateLog();
    void logUpdate(const QString& msg);

    QString m_rootPath;
    QString m_dataPath;
    bool m_isPortable = false;
    bool m_isAppimage = false;
    bool m_isFlatpak = false;
    QString m_appimagePath;
    QString m_prismExecutable;
    QUrl m_prismRepoUrl;
    Version m_userSelectedVersion;
    bool m_checkOnly;
    bool m_forceUpdate;
    bool m_printOnly;
    bool m_selectUI;
    bool m_allowDowngrade;
    bool m_allowPreRelease;

    QString m_updateLogPath;

    QString m_prismBinaryName;
    QString m_prismVersion;
    int m_prismVersionMajor = -1;
    int m_prismVersionMinor = -1;
    int m_prismVersionPatch = -1;
    QString m_prsimVersionChannel;
    QString m_prismGitCommit;

    GitHubRelease m_install_release;

    Status m_status = Status::Starting;
    std::unique_ptr<QNetworkAccessManager> m_network;
    QString m_current_url;
    Task::Ptr m_current_task;
    QList<GitHubRelease> m_releases;

   public:
    std::unique_ptr<QFile> logFile;
    bool logToConsole = false;

#if defined Q_OS_WIN32
    // used on Windows to attach the standard IO streams
    bool consoleAttached = false;
#endif
};
