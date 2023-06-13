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

#pragma once

#include <QtCore>

#include <QApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFlag>
#include <QIcon>
#include <QLocalSocket>
#include <QUrl>
#include <memory>

#define PRISM_EXTERNAL_EXE
#include "FileSystem.h"

class FileLinkApp : public QCoreApplication {
    // friends for the purpose of limiting access to deprecated stuff
    Q_OBJECT
   public:
    FileLinkApp(int& argc, char** argv);
    virtual ~FileLinkApp();

   private:
    void joinServer(QString server);
    void readPathPairs();
    void runLink();
    void sendResults();

    bool m_useHardLinks = false;

    QDateTime m_startTime;
    QLocalSocket socket;
    QDataStream in;
    quint32 blockSize;

    QList<FS::LinkPair> m_links_to_make;
    QList<FS::LinkResult> m_path_results;

#if defined Q_OS_WIN32
    // used on Windows to attach the standard IO streams
    bool consoleAttached = false;
#endif
};
