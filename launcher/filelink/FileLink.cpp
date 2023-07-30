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

#include "StringUtils.h"

#include <iostream>

#include <QAccessible>
#include <QCommandLineParser>

#include <QDebug>

#include <DesktopServices.h>

#include <sys.h>

#if defined Q_OS_WIN32
#include "WindowsConsole.h"
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

#if defined Q_OS_WIN32

// taken from https://stackoverflow.com/a/25927081
// getting a proper output to console with redirection support on windows is apparently hell
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

FileLinkApp::FileLinkApp(int& argc, char** argv) : QCoreApplication(argc, argv), socket(new QLocalSocket(this))
{
#if defined Q_OS_WIN32
    // attach the parent console
    if (AttachWindowsConsole()) {
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

    parser.addOptions({ { { "s", "server" }, "Join the specified server on launch", "pipe name" },
                        { { "H", "hard" }, "use hard links instead of symbolic", "true/false" } });
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(arguments());

    QString serverToJoin = parser.value("server");
    m_useHardLinks = QVariant(parser.value("hard")).toBool();

    qDebug() << "link program launched";

    if (!serverToJoin.isEmpty()) {
        qDebug() << "joining server" << serverToJoin;
        joinServer(serverToJoin);
    } else {
        qDebug() << "no server to join";
        m_status = Failed;
        exit();
    }
}

void FileLinkApp::joinServer(QString server)
{
    blockSize = 0;

    in.setDevice(&socket);

    connect(&socket, &QLocalSocket::connected, this, [&]() { qDebug() << "connected to server"; });

    connect(&socket, &QLocalSocket::readyRead, this, &FileLinkApp::readPathPairs);

    connect(&socket, &QLocalSocket::errorOccurred, this, [&](QLocalSocket::LocalSocketError socketError) {
        m_status = Failed;
        switch (socketError) {
            case QLocalSocket::ServerNotFoundError:
                qDebug()
                    << ("The host was not found. Please make sure "
                        "that the server is running and that the "
                        "server name is correct.");
                break;
            case QLocalSocket::ConnectionRefusedError:
                qDebug()
                    << ("The connection was refused by the peer. "
                        "Make sure the server is running, "
                        "and check that the server name "
                        "is correct.");
                break;
            case QLocalSocket::PeerClosedError:
                qDebug() << ("The connection was closed by the peer. ");
                break;
            default:
                qDebug() << "The following error occurred: " << socket.errorString();
        }
    });

    connect(&socket, &QLocalSocket::disconnected, this, [&]() {
        qDebug() << "disconnected from server, should exit";
        m_status = Succeeded;
        exit();
    });

    socket.connectToServer(server);
}

void FileLinkApp::runLink()
{
    std::error_code os_err;

    qDebug() << "creating links";

    for (auto link : m_links_to_make) {
        QString src_path = link.src;
        QString dst_path = link.dst;

        FS::ensureFilePathExists(dst_path);
        if (m_useHardLinks) {
            qDebug() << "making hard link:" << src_path << "to" << dst_path;
            fs::create_hard_link(StringUtils::toStdString(src_path), StringUtils::toStdString(dst_path), os_err);
        } else if (fs::is_directory(StringUtils::toStdString(src_path))) {
            qDebug() << "making directory_symlink:" << src_path << "to" << dst_path;
            fs::create_directory_symlink(StringUtils::toStdString(src_path), StringUtils::toStdString(dst_path), os_err);
        } else {
            qDebug() << "making symlink:" << src_path << "to" << dst_path;
            fs::create_symlink(StringUtils::toStdString(src_path), StringUtils::toStdString(dst_path), os_err);
        }

        if (os_err) {
            qWarning() << "Failed to link files:" << QString::fromStdString(os_err.message());
            qDebug() << "Source file:" << src_path;
            qDebug() << "Destination file:" << dst_path;
            qDebug() << "Error category:" << os_err.category().name();
            qDebug() << "Error code:" << os_err.value();

            FS::LinkResult result = { src_path, dst_path, QString::fromStdString(os_err.message()), os_err.value() };
            m_path_results.append(result);
        } else {
            FS::LinkResult result = { src_path, dst_path, "", 0};
            m_path_results.append(result);
        }
    }

    sendResults();
    qDebug() << "done, should exit soon";
}

void FileLinkApp::sendResults()
{
    // construct block of data to send
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    qint32 blocksize = quint32(sizeof(quint32));
    for (auto result : m_path_results) {
        blocksize += quint32(result.src.size());
        blocksize += quint32(result.dst.size());
        blocksize += quint32(result.err_msg.size());
        blocksize += quint32(sizeof(quint32));
    }
    qDebug() << "About to write block of size:" << blocksize;
    out << blocksize;

    out << quint32(m_path_results.length());
    for (auto result : m_path_results) {
        out << result.src;
        out << result.dst;
        out << result.err_msg;
        out << quint32(result.err_value);
    }

    qint64 byteswritten = socket.write(block);
    bool bytesflushed = socket.flush();
    qDebug() << "block flushed" << byteswritten << bytesflushed;
}

void FileLinkApp::readPathPairs()
{
    m_links_to_make.clear();
    qDebug() << "Reading path pairs from server";
    qDebug() << "bytes available" << socket.bytesAvailable();
    if (blockSize == 0) {
        // Relies on the fact that QDataStream serializes a quint32 into
        // sizeof(quint32) bytes
        if (socket.bytesAvailable() < (int)sizeof(quint32))
            return;
        qDebug() << "reading block size";
        in >> blockSize;
    }
    qDebug() << "blocksize is" << blockSize;
    qDebug() << "bytes available" << socket.bytesAvailable();
    if (socket.bytesAvailable() < blockSize || in.atEnd())
        return;

    quint32 numLinks;
    in >> numLinks;
    qDebug() << "numLinks" << numLinks;

    for (unsigned int i = 0; i < numLinks; i++) {
        FS::LinkPair pair;
        in >> pair.src;
        in >> pair.dst;
        qDebug() << "link" << pair.src << "to" << pair.dst;
        m_links_to_make.append(pair);
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
    if (consoleAttached) {
        fclose(stdout);
        fclose(stdin);
        fclose(stderr);
    }
#endif
}
