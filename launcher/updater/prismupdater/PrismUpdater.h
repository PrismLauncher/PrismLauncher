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

#define PRISM_EXTERNAL_EXE
#include "FileSystem.h"

#include "updater/prismupdater/GitHubRelease.h"

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

    void loadPrismVersionFromExe(const QString& exe_path);
    
    void downloadReleasePage(const QString& api_url, int page);
    int parseReleasePage(const QByteArray* responce);
    GitHubRelease getLatestRelease();
    bool needUpdate(const GitHubRelease& release);
    GitHubRelease selectRelease();
    void performUpdate(const GitHubRelease& release);
    void printReleases();
    QList<GitHubRelease> newerReleases();
    QList<GitHubRelease> nonDraftReleases();

    void downloadError(QNetworkReply::NetworkError error);
    void sslErrors(const QList<QSslError>& errors);

    const QString& root() { return m_rootPath; }

    bool isPortable() { return m_portable; }

    QString m_rootPath;
    bool m_portable = false;
    QString m_prismExecutable;
    QUrl m_prismRepoUrl;
    Version m_userSelectedVersion;
    bool m_checkOnly;
    bool m_forceUpdate;
    bool m_printOnly;
    bool m_updateLatest;
    bool m_allowDowngrade;

    QString m_prismBinaryName;
    QString m_prismVerison;
    int m_prismVersionMajor;
    int m_prismVersionMinor;
    QString m_prsimVersionChannel;
    QString m_prismGitCommit;

    Status m_status = Status::Starting;
    QNetworkAccessManager* m_network;
    QNetworkReply* m_reply;
    QList<GitHubRelease> m_releases;

   public:
    std::unique_ptr<QFile> logFile;
    bool logToConsole = false;

#if defined Q_OS_WIN32
    // used on Windows to attach the standard IO streams
    bool consoleAttached = false;
#endif
};
