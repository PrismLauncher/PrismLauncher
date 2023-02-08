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

#include "FileLink.h"
#include "BuildConfig.h"


#include <iostream>

#include <QAccessible>
#include <QCommandLineParser>

#include <QDebug>

#include <DesktopServices.h>

#include <sys.h>

#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#endif




FileLinkApp::FileLinkApp(int &argc, char **argv) : QCoreApplication(argc, argv), socket(new QLocalSocket(this))
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
    setApplicationName(BuildConfig.LAUNCHER_NAME + "FileLink");
    setApplicationVersion(BuildConfig.printableVersionString() + "\n" + BuildConfig.GIT_COMMIT);

    // Commandline parsing
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("a batch MKLINK program for windows to be used with prismlauncher"));

    parser.addOptions({
        {{"s", "server"}, "Join the specified server on launch", "pipe name"}
    });
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(arguments());

    QString serverToJoin = parser.value("server");

    qDebug() << "link program launched";

    if (!serverToJoin.isEmpty()) {
        qDebug() << "joining server" << serverToJoin;
        joinServer(serverToJoin);
    } else {
        qDebug() << "no server to join";
        exit();
    }

}

void FileLinkApp::joinServer(QString server)
{
    
    blockSize = 0; 

    in.setDevice(&socket);
    in.setVersion(QDataStream::Qt_5_15);

    connect(&socket, &QLocalSocket::connected, this, [&](){
        qDebug() << "connected to server";
    });

    connect(&socket, &QLocalSocket::readyRead, this, &FileLinkApp::readPathPairs);

    connect(&socket, &QLocalSocket::errorOccurred, this, [&](QLocalSocket::LocalSocketError socketError){
        switch (socketError) {
        case QLocalSocket::ServerNotFoundError:
            qDebug() << tr("The host was not found. Please make sure "
                            "that the server is running and that the "
                            "server name is correct.");
            break;
        case QLocalSocket::ConnectionRefusedError:
            qDebug() << tr("The connection was refused by the peer. "
                            "Make sure the server is running, "
                            "and check that the server name "
                            "is correct.");
            break;
        case QLocalSocket::PeerClosedError:
            break;
        default:
            qDebug() << tr("The following error occurred: %1.").arg(socket.errorString());
        }
    });

    connect(&socket, &QLocalSocket::disconnected, this, [&](){
        qDebug() << "dissconnected from server";
    });

    socket.connectToServer(server);


}

void FileLinkApp::runLink()
{
    qDebug() << "creating link";
    FS::create_link lnk(m_path_pairs);
    lnk.debug(true);
    if (!lnk()) {
        qDebug() << "Link Failed!" << lnk.getOSError().value() << lnk.getOSError().message().c_str();
    }
    //exit();
    qDebug() << "done, should exit";
}

void FileLinkApp::readPathPairs()
{   
    m_path_pairs.clear();
    qDebug() << "Reading path pairs from server";
    qDebug() << "bytes avalible" << socket.bytesAvailable();
    if (blockSize == 0) {
        // Relies on the fact that QDataStream serializes a quint32 into
        // sizeof(quint32) bytes
        if (socket.bytesAvailable() < (int)sizeof(quint32))
            return;
        qDebug() << "reading block size";
        in >> blockSize;
    }
    qDebug() << "blocksize is" << blockSize;
    qDebug() << "bytes avalible" << socket.bytesAvailable();
    if (socket.bytesAvailable() < blockSize || in.atEnd())
        return;
    
    quint32 numPairs;
    in >> numPairs;
    qDebug() << "numPairs" << numPairs;

    for(int i = 0; i < numPairs; i++) {
        FS::LinkPair pair;
        in >> pair.src;
        in >> pair.dst;
        qDebug() << "link" << pair.src << "to" << pair.dst;
        m_path_pairs.append(pair);
    }

    runLink();
}


FileLinkApp::~FileLinkApp()
{   
    qDebug() << "link program shutting down";
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




