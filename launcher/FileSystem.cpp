// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

#include "FileSystem.h"
#include <qdebug.h>
#include <qfileinfo.h>
#include <qnamespace.h>
#include <qstorageinfo.h>

#include "BuildConfig.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QtNetwork>
#include <system_error>

#include "DesktopServices.h"
#include "StringUtils.h"

#if defined Q_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <sys/utime.h>
#include <versionhelpers.h>
#include <windows.h>
#include <winnls.h>
#include <string>
//for ShellExecute
#include <shlobj.h>
#include <objbase.h>
#include <Shellapi.h>
#else
#include <utime.h>
#endif

// Snippet from https://github.com/gulrak/filesystem#using-it-as-single-file-header

#ifdef __APPLE__
#include <Availability.h> // for deployment target to support pre-catalina targets without std::fs
#endif // __APPLE__

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || (defined(__cplusplus) && __cplusplus >= 201703L)) && defined(__has_include)
#if __has_include(<filesystem>) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif // MacOS min version check
#endif // Other OSes version check

#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif


// clone
#if defined(Q_OS_LINUX)
#include <linux/fs.h>
#include <fcntl.h>      /* Definition of FICLONE* constants */
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#elif defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
#include <sys/attr.h>
#include <sys/clonefile.h>
#elif defined(Q_OS_WIN)
// winbtrfs clone vs rundll32 shellbtrfs.dll,ReflinkCopy
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
// refs
#include <winioctl.h>
#endif

namespace FS {

void ensureExists(const QDir& dir)
{
    if (!QDir().mkpath(dir.absolutePath())) {
        throw FileSystemException("Unable to create folder " + dir.dirName() + " (" + dir.absolutePath() + ")");
    }
}

void write(const QString& filename, const QByteArray& data)
{
    ensureExists(QFileInfo(filename).dir());
    QSaveFile file(filename);
    if (!file.open(QSaveFile::WriteOnly)) {
        throw FileSystemException("Couldn't open " + filename + " for writing: " + file.errorString());
    }
    if (data.size() != file.write(data)) {
        throw FileSystemException("Error writing data to " + filename + ": " + file.errorString());
    }
    if (!file.commit()) {
        throw FileSystemException("Error while committing data to " + filename + ": " + file.errorString());
    }
}

QByteArray read(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        throw FileSystemException("Unable to open " + filename + " for reading: " + file.errorString());
    }
    const qint64 size = file.size();
    QByteArray data(int(size), 0);
    const qint64 ret = file.read(data.data(), size);
    if (ret == -1 || ret != size) {
        throw FileSystemException("Error reading data from " + filename + ": " + file.errorString());
    }
    return data;
}

bool updateTimestamp(const QString& filename)
{
#ifdef Q_OS_WIN32
    std::wstring filename_utf_16 = filename.toStdWString();
    return (_wutime64(filename_utf_16.c_str(), nullptr) == 0);
#else
    QByteArray filenameBA = QFile::encodeName(filename);
    return (utime(filenameBA.data(), nullptr) == 0);
#endif
}

bool ensureFilePathExists(QString filenamepath)
{
    QFileInfo a(filenamepath);
    QDir dir;
    QString ensuredPath = a.path();
    bool success = dir.mkpath(ensuredPath);
    return success;
}

bool ensureFolderPathExists(QString foldernamepath)
{
    QFileInfo a(foldernamepath);
    QDir dir;
    QString ensuredPath = a.filePath();
    bool success = dir.mkpath(ensuredPath);
    return success;
}

/**
 * @brief Copies a directory and it's contents from src to dest
 * @param offset subdirectory form src to copy to dest
 * @return if there was an error during the filecopy
 */
bool copy::operator()(const QString& offset, bool dryRun)
{
    using copy_opts = fs::copy_options;
    m_copied = 0;  // reset counter

// NOTE always deep copy on windows. the alternatives are too messy.
#if defined Q_OS_WIN32
    m_followSymlinks = true;
#endif

    auto src = PathCombine(m_src.absolutePath(), offset);
    auto dst = PathCombine(m_dst.absolutePath(), offset);

    std::error_code err;

    fs::copy_options opt = copy_opts::none;

    // The default behavior is to follow symlinks
    if (!m_followSymlinks)
        opt |= copy_opts::copy_symlinks;

    // Function that'll do the actual copying
    auto copy_file = [&](QString src_path, QString relative_dst_path) {
        if (m_matcher && (m_matcher->matches(relative_dst_path) != m_whitelist))
            return;

        auto dst_path = PathCombine(dst, relative_dst_path);
        if (!dryRun) {
            ensureFilePathExists(dst_path);
            fs::copy(StringUtils::toStdString(src_path), StringUtils::toStdString(dst_path), opt, err);
        }
        if (err) {
            qWarning() << "Failed to copy files:" << QString::fromStdString(err.message());
            qDebug() << "Source file:" << src_path;
            qDebug() << "Destination file:" << dst_path;
        }
        m_copied++;
        emit fileCopied(relative_dst_path);
    };

    // We can't use copy_opts::recursive because we need to take into account the
    // blacklisted paths, so we iterate over the source directory, and if there's no blacklist
    // match, we copy the file.
    QDir src_dir(src);
    QDirIterator source_it(src, QDir::Filter::Files | QDir::Filter::Hidden, QDirIterator::Subdirectories);

    while (source_it.hasNext()) {
        auto src_path = source_it.next();
        auto relative_path = src_dir.relativeFilePath(src_path);

        copy_file(src_path, relative_path);
    }

    // If the root src is not a directory, the previous iterator won't run.
    if (!fs::is_directory(StringUtils::toStdString(src)))
        copy_file(src, "");

    return err.value() == 0;
}

/// qDebug print support for the LinkPair struct
QDebug operator<<(QDebug debug, const LinkPair& lp)
{
    QDebugStateSaver saver(debug);

    debug.nospace() << "LinkPair{ src: " << lp.src << " , dst: " << lp.dst << " }";
    return debug;
}

bool create_link::operator()(const QString& offset, bool dryRun) 
{
    m_linked = 0;  // reset counter
    m_path_results.clear();
    m_links_to_make.clear();

    m_path_results.clear();
    
    make_link_list(offset);
    
    if (!dryRun)
        return make_links();

    return true;
}


/**
 * @brief make a list off all the  links ot make
 * @param offset subdirectory form src to link to dest
 * @return if there was an error during the attempt to link
 */
void create_link::make_link_list(const QString& offset)
{
    for (auto pair : m_path_pairs) {
        const QString& srcPath = pair.src;
        const QString& dstPath = pair.dst;

        auto src = PathCombine(QDir(srcPath).absolutePath(), offset);
        auto dst = PathCombine(QDir(dstPath).absolutePath(), offset);

        // you can't hard link a directory so make sure if we deal with a directory we do so recursively
        if (m_useHardLinks)
            m_recursive = true;

        // Function that'll do the actual linking
        auto link_file = [&](QString src_path, QString relative_dst_path) {
            if (m_matcher && (m_matcher->matches(relative_dst_path) != m_whitelist)) {
                qDebug() << "path" << relative_dst_path << "in black list or not in whitelist";
                return;
            }
                

            auto dst_path = PathCombine(dst, relative_dst_path);
            LinkPair link = {src_path, dst_path};
            m_links_to_make.append(link); 
        };
        
        if ((!m_recursive) || !fs::is_directory(StringUtils::toStdString(src))) {
            if (m_debug)
                qDebug() << "linking single file or dir:" << src << "to" << dst;
            link_file(src, "");
        } else {
            if (m_debug)
                qDebug() << "linking recursivly:" << src << "to" << dst << "max_depth:" << m_max_depth;
            QDir src_dir(src);
            QDirIterator source_it(src, QDir::Filter::Files | QDir::Filter::Hidden, QDirIterator::Subdirectories);

            QStringList linkedPaths;

            while (source_it.hasNext()) {
                auto src_path = source_it.next();
                auto relative_path = src_dir.relativeFilePath(src_path);

                if (m_max_depth >= 0 && PathDepth(relative_path) > m_max_depth){
                    relative_path = PathTruncate(relative_path, m_max_depth);
                    src_path = src_dir.filePath(relative_path);
                    if (linkedPaths.contains(src_path)) {
                        continue;
                    }
                }

                linkedPaths.append(src_path);

                link_file(src_path, relative_path);
            }
        }
    }  
}

bool create_link::make_links()
{   
    for (auto link : m_links_to_make) {

        QString src_path = link.src;
        QString dst_path = link.dst;

        ensureFilePathExists(dst_path);
        if (m_useHardLinks) {
            if (m_debug)
                qDebug() << "making hard link:" << src_path << "to" << dst_path;
            fs::create_hard_link(StringUtils::toStdString(src_path), StringUtils::toStdString(dst_path), m_os_err);
        } else if (fs::is_directory(StringUtils::toStdString(src_path))) {
            if (m_debug)
                qDebug() << "making directory_symlink:" << src_path << "to" << dst_path;
            fs::create_directory_symlink(StringUtils::toStdString(src_path), StringUtils::toStdString(dst_path), m_os_err);
        } else {
            if (m_debug)
                qDebug() << "making symlink:" << src_path << "to" << dst_path;
            fs::create_symlink(StringUtils::toStdString(src_path), StringUtils::toStdString(dst_path), m_os_err);
        }
           

        if (m_os_err) {
            qWarning() << "Failed to link files:" << QString::fromStdString(m_os_err.message());
            qDebug() << "Source file:" << src_path;
            qDebug() << "Destination file:" << dst_path;
            qDebug() << "Error catagory:" << m_os_err.category().name();
            qDebug() << "Error code:" << m_os_err.value();
            emit linkFailed(src_path, dst_path, QString::fromStdString(m_os_err.message()), m_os_err.value());
        } else {
            m_linked++;
            emit fileLinked(src_path, dst_path);
        }
        if (m_os_err) return false;
    }
    return true;
}

void create_link::runPrivlaged(const QString& offset)
{
    m_linked = 0;  // reset counter
    m_path_results.clear();
    m_links_to_make.clear();

    bool gotResults = false;

    make_link_list(offset);

    QString serverName = BuildConfig.LAUNCHER_APP_BINARY_NAME + "_filelink_server" + StringUtils::getRandomAlphaNumeric(8);

    connect(&m_linkServer, &QLocalServer::newConnection, this, [&](){

        qDebug() << "Client connected, sending out pairs";
        // construct block of data to send
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_0); // choose correct version better?

        qint32 blocksize = quint32(sizeof(quint32));
        for (auto link : m_links_to_make) {
            blocksize += quint32(link.src.size()); 
            blocksize += quint32(link.dst.size());
        }
        qDebug() << "About to write block of size:" << blocksize;
        out << blocksize;

        out << quint32(m_links_to_make.length());
        for (auto link : m_links_to_make) {
            out << link.src;   
            out << link.dst;
        }

        QLocalSocket *clientConnection = m_linkServer.nextPendingConnection();
        connect(clientConnection, &QLocalSocket::disconnected,
                clientConnection, &QLocalSocket::deleteLater);
        
        connect(clientConnection, &QLocalSocket::readyRead, this, [&, clientConnection](){
            QDataStream in;
            quint32 blockSize = 0;
            in.setDevice(clientConnection);
            in.setVersion(QDataStream::Qt_5_0);
            qDebug() << "Reading path results from client";
            qDebug() << "bytes avalible" << clientConnection->bytesAvailable();

            // Relies on the fact that QDataStream serializes a quint32 into
            // sizeof(quint32) bytes
            if (clientConnection->bytesAvailable() < (int)sizeof(quint32))
                return;
            qDebug() << "reading block size";
            in >> blockSize;

            qDebug() << "blocksize is" << blockSize;
            qDebug() << "bytes avalible" << clientConnection->bytesAvailable();
            if (clientConnection->bytesAvailable() < blockSize || in.atEnd())
                return;
            
            quint32 numResults;
            in >> numResults;
            qDebug() << "numResults" << numResults;

            for(quint32 i = 0; i < numResults; i++) {
                FS::LinkResult result;
                in >> result.src;
                in >> result.dst;
                in >> result.err_msg;
                qint32 err_value;
                in >>  err_value;
                result.err_value = err_value;
                if (result.err_value) {
                    qDebug() << "privlaged link fail" << result.src << "to" << result.dst << "code" << result.err_value << result.err_msg;
                    emit linkFailed(result.src, result.dst, result.err_msg, result.err_value);
                } else {
                    qDebug() << "privlaged link success" << result.src << "to" << result.dst;
                    m_linked++;
                    emit fileLinked(result.src, result.dst);
                }
                m_path_results.append(result);
            }
            gotResults = true;
            qDebug() << "results recieved, closing connection";
            clientConnection->close();
        });

        qint64 byteswritten = clientConnection->write(block);
        bool bytesflushed = clientConnection->flush();
        qDebug() << "block flushed" << byteswritten << bytesflushed;

    });

    qDebug() << "Listening on pipe" << serverName;
    if (!m_linkServer.listen(serverName)) {
        qDebug() << "Unable to start local pipe server on" << serverName << ":" << m_linkServer.errorString();
        return;
    }

    ExternalLinkFileProcess* linkFileProcess = new ExternalLinkFileProcess(serverName, m_useHardLinks, this);
    connect(linkFileProcess, &ExternalLinkFileProcess::processExited, this, [&]() { emit finishedPrivlaged(gotResults); });
    connect(linkFileProcess, &ExternalLinkFileProcess::finished, linkFileProcess, &QObject::deleteLater);

    linkFileProcess->start();

}

void ExternalLinkFileProcess::runLinkFile() {
    QString fileLinkExe = PathCombine(QCoreApplication::instance()->applicationDirPath(),  BuildConfig.LAUNCHER_APP_BINARY_NAME + "_filelink");
    QString params = "-s " + m_server;

    params += " -H " + QVariant(m_useHardLinks).toString();

#if defined Q_OS_WIN32
    SHELLEXECUTEINFO ShExecInfo;

    fileLinkExe = fileLinkExe + ".exe";

    qDebug() << "Running: runas" << fileLinkExe << params;

    LPCWSTR programNameWin = (const wchar_t*) fileLinkExe.utf16();
    LPCWSTR paramsWin = (const wchar_t*) params.utf16();

    // https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-shellexecuteinfoa
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL; // Optional. A handle to the owner window, used to display and position any UI that the system might produce while executing this function.
    ShExecInfo.lpVerb = L"runas"; // elevate to admin, show UAC
    ShExecInfo.lpFile = programNameWin;
    ShExecInfo.lpParameters = paramsWin;
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_HIDE;
    ShExecInfo.hInstApp = NULL;

    ShellExecuteEx(&ShExecInfo);

    WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
    CloseHandle(ShExecInfo.hProcess);
#endif

    qDebug() << "Process exited";
}

bool move(const QString& source, const QString& dest)
{
    std::error_code err;

    ensureFilePathExists(dest);
    fs::rename(StringUtils::toStdString(source), StringUtils::toStdString(dest), err);

    if (err) {
        qWarning() << "Failed to move file:" << QString::fromStdString(err.message());
        qDebug() << "Source file:" << source;
        qDebug() << "Destination file:" << dest;
    }

    return err.value() == 0;
}

bool deletePath(QString path)
{
    std::error_code err;

    fs::remove_all(StringUtils::toStdString(path), err);

    if (err) {
        qWarning() << "Failed to remove files:" << QString::fromStdString(err.message());
    }

    return err.value() == 0;
}

bool trash(QString path, QString *pathInTrash)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    return false;
#else
    // FIXME: Figure out trash in Flatpak. Qt seemingly doesn't use the Trash portal
    if (DesktopServices::isFlatpak())
        return false;
#if defined Q_OS_WIN32
    if (IsWindowsServer())
        return false;
#endif
    return QFile::moveToTrash(path, pathInTrash);
#endif
}

QString PathCombine(const QString& path1, const QString& path2)
{
    if (!path1.size())
        return path2;
    if (!path2.size())
        return path1;
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

QString PathCombine(const QString& path1, const QString& path2, const QString& path3)
{
    return PathCombine(PathCombine(path1, path2), path3);
}

QString PathCombine(const QString& path1, const QString& path2, const QString& path3, const QString& path4)
{
    return PathCombine(PathCombine(path1, path2, path3), path4);
}

QString AbsolutePath(const QString& path)
{
    return QFileInfo(path).absolutePath();
}

int PathDepth(const QString& path)
{
    if (path.isEmpty()) return 0;

    QFileInfo info(path);

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    auto parts = QDir::toNativeSeparators(info.path()).split(QDir::separator(), QString::SkipEmptyParts);
#else
    auto parts = QDir::toNativeSeparators(info.path()).split(QDir::separator(), Qt::SkipEmptyParts);
#endif

    int numParts = parts.length();
    numParts -= parts.count(".");
    numParts -= parts.count("..") * 2;
    
    return numParts;
}

QString PathTruncate(const QString& path, int depth)
{
    if (path.isEmpty() || (depth < 0) ) return "";

    QString trunc = QFileInfo(path).path();

    if (PathDepth(trunc) > depth ) {
        return PathTruncate(trunc, depth);
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    auto parts = QDir::toNativeSeparators(trunc).split(QDir::separator(), QString::SkipEmptyParts);
#else
    auto parts = QDir::toNativeSeparators(trunc).split(QDir::separator(), Qt::SkipEmptyParts);
#endif

    if (parts.startsWith(".") && !path.startsWith(".")) {
        parts.removeFirst();
    }
    if (QDir::toNativeSeparators(path).startsWith(QDir::separator())) {
        parts.prepend("");
    }

    trunc = parts.join(QDir::separator());

    return trunc;
}

QString ResolveExecutable(QString path)
{
    if (path.isEmpty()) {
        return QString();
    }
    if (!path.contains('/')) {
        path = QStandardPaths::findExecutable(path);
    }
    QFileInfo pathInfo(path);
    if (!pathInfo.exists() || !pathInfo.isExecutable()) {
        return QString();
    }
    return pathInfo.absoluteFilePath();
}

/**
 * Normalize path
 *
 * Any paths inside the current folder will be normalized to relative paths (to current)
 * Other paths will be made absolute
 */
QString NormalizePath(QString path)
{
    QDir a = QDir::currentPath();
    QString currentAbsolute = a.absolutePath();

    QDir b(path);
    QString newAbsolute = b.absolutePath();

    if (newAbsolute.startsWith(currentAbsolute)) {
        return a.relativeFilePath(newAbsolute);
    } else {
        return newAbsolute;
    }
}

QString badFilenameChars = "\"\\/?<>:;*|!+\r\n";

QString RemoveInvalidFilenameChars(QString string, QChar replaceWith)
{
    for (int i = 0; i < string.length(); i++) {
        if (badFilenameChars.contains(string[i])) {
            string[i] = replaceWith;
        }
    }
    return string;
}

QString DirNameFromString(QString string, QString inDir)
{
    int num = 0;
    QString baseName = RemoveInvalidFilenameChars(string, '-');
    QString dirName;
    do {
        if (num == 0) {
            dirName = baseName;
        } else {
            dirName = baseName + "(" + QString::number(num) + ")";
        }

        // If it's over 9000
        if (num > 9000)
            return "";
        num++;
    } while (QFileInfo(PathCombine(inDir, dirName)).exists());
    return dirName;
}

// Does the folder path contain any '!'? If yes, return true, otherwise false.
// (This is a problem for Java)
bool checkProblemticPathJava(QDir folder)
{
    QString pathfoldername = folder.absolutePath();
    return pathfoldername.contains("!", Qt::CaseInsensitive);
}

QString getDesktopDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

// Cross-platform Shortcut creation
bool createShortcut(QString destination, QString target, QStringList args, QString name, QString icon)
{
#if defined(Q_OS_MACOS)
    destination += ".command";

    QFile f(destination);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&f);

    QString argstring;
    if (!args.empty())
        argstring = " \"" + args.join("\" \"") + "\"";

    stream << "#!/bin/bash"
           << "\n";
    stream << "\""
           << target
           << "\" "
           << argstring
           << "\n";

    stream.flush();
    f.close();

    f.setPermissions(f.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);

    return true;
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    QFile f(destination);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&f);

    QString argstring;
    if (!args.empty())
        argstring = " '" + args.join("' '") + "'";

    stream << "[Desktop Entry]"
           << "\n";
    stream << "Type=Application"
           << "\n";
    stream << "Exec=\"" << target.toLocal8Bit() << "\"" << argstring.toLocal8Bit() << "\n";
    stream << "Name=" << name.toLocal8Bit() << "\n";
    if (!icon.isEmpty())
    {
        stream << "Icon=" << icon.toLocal8Bit() << "\n";
    }

    stream.flush();
    f.close();

    f.setPermissions(f.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);

    return true;
#elif defined(Q_OS_WIN)
    QFileInfo targetInfo(target);

    if (!targetInfo.exists())
    {
        qWarning() << "Target file does not exist!";
        return false;
    }

    target = targetInfo.absoluteFilePath();

    if (target.length() >= MAX_PATH)
    {
        qWarning() << "Target file path is too long!";
        return false;
    }

    if (!icon.isEmpty() && icon.length() >= MAX_PATH)
    {
        qWarning() << "Icon path is too long!";
        return false;
    }

    destination += ".lnk";

    if (destination.length() >= MAX_PATH)
    {
        qWarning() << "Destination path is too long!";
        return false;
    }

    QString argStr;
    int argCount = args.count();
    for (int i = 0; i < argCount; i++)
    {
        if (args[i].contains(' '))
        {
            argStr.append('"').append(args[i]).append('"');
        }
        else
        {
            argStr.append(args[i]);
        }

        if (i < argCount - 1)
        {
            argStr.append(" ");
        }
    }

    if (argStr.length() >= MAX_PATH)
    {
        qWarning() << "Arguments string is too long!";
        return false;
    }

    HRESULT hres;

    // ...yes, you need to initialize the entire COM stack just to make a shortcut
    hres = CoInitialize(nullptr);
    if (FAILED(hres))
    {
        qWarning() << "Failed to initialize COM!";
        return false;
    }

    WCHAR wsz[MAX_PATH];

    IShellLink* psl;

    // create an IShellLink instance - this stores the shortcut's attributes
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres))
    {
        wmemset(wsz, 0, MAX_PATH);
        target.toWCharArray(wsz);
        psl->SetPath(wsz);

        wmemset(wsz, 0, MAX_PATH);
        argStr.toWCharArray(wsz);
        psl->SetArguments(wsz);

        wmemset(wsz, 0, MAX_PATH);
        targetInfo.absolutePath().toWCharArray(wsz);
        psl->SetWorkingDirectory(wsz); // "Starts in" attribute

        if (!icon.isEmpty())
        {
            wmemset(wsz, 0, MAX_PATH);
            icon.toWCharArray(wsz);
            psl->SetIconLocation(wsz, 0);
        }

        // query an IPersistFile interface from our IShellLink instance
        // this is the interface that will actually let us save the shortcut to disk!
        IPersistFile* ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres))
        {
            wmemset(wsz, 0, MAX_PATH);
            destination.toWCharArray(wsz);
            hres = ppf->Save(wsz, TRUE);
            if (FAILED(hres))
            {
                qWarning() << "IPresistFile->Save() failed";
                qWarning() << "hres = " << hres;
            }
            ppf->Release();
        }
        else
        {
            qWarning() << "Failed to query IPersistFile interface from IShellLink instance";
            qWarning() << "hres = " << hres;
        }
        psl->Release();
    }
    else
    {
        qWarning() << "Failed to create IShellLink instance";
        qWarning() << "hres = " << hres;
    }

    // go away COM, nobody likes you
    CoUninitialize();

    return SUCCEEDED(hres);
#else
    qWarning("Desktop Shortcuts not supported on your platform!");
    return false;
#endif
}

bool overrideFolder(QString overwritten_path, QString override_path)
{
    using copy_opts = fs::copy_options;

    if (!FS::ensureFolderPathExists(overwritten_path))
        return false;

    std::error_code err;
    fs::copy_options opt = copy_opts::recursive | copy_opts::overwrite_existing;

    // FIXME: hello traveller! Apparently std::copy does NOT overwrite existing files on GNU libstdc++ on Windows?
    fs::copy(StringUtils::toStdString(override_path), StringUtils::toStdString(overwritten_path), opt, err);

    if (err) {
        qCritical() << QString("Failed to apply override from %1 to %2").arg(override_path, overwritten_path);
        qCritical() << "Reason:" << QString::fromStdString(err.message());
    }

    return err.value() == 0;
}

/**
 * @brief path to the near ancestor that exsists
 * 
 */
QString NearestExistentAncestor(const QString& path)
{
    if(QFileInfo::exists(path)) return path;

    QDir dir(path);
    if(!dir.makeAbsolute()) return {};
    do
    {
        dir.setPath(QDir::cleanPath(dir.filePath(QStringLiteral(".."))));
    }
    while(!dir.exists() && !dir.isRoot());

    return dir.exists() ? dir.path() : QString();
}

/**
 * @brief colect information about the filesystem under a file
 * 
 */
FilesystemInfo statFS(const QString& path)
{

    FilesystemInfo info;

    QStorageInfo storage_info(NearestExistentAncestor(path));

    QString fsTypeName = QString::fromStdString(storage_info.fileSystemType().toStdString());
    qDebug() << "Qt reports Filesystem at" << path << "root:" << storage_info.rootPath() << "as" << fsTypeName;

    for (auto fs_type_pair : s_filesystem_type_names_inverse.toStdMap()) {
        auto fs_type_name = fs_type_pair.first;
        auto fs_type = fs_type_pair.second;

        if(fsTypeName.toLower().contains(fs_type_name.toLower())) {
            info.fsType = fs_type;
            break;
        }
    }

    info.blockSize = storage_info.blockSize();
    info.bytesAvailable = storage_info.bytesAvailable();
    info.bytesFree = storage_info.bytesFree();
    info.bytesTotal = storage_info.bytesTotal();

    info.name = storage_info.name();
    info.rootPath = storage_info.rootPath();

    return info;
}

/**
 * @brief if the Filesystem is reflink/clone capable 
 * 
 */
bool canCloneOnFS(const QString& path)
{   
    FilesystemInfo info = statFS(path);
    return canCloneOnFS(info);
}
bool canCloneOnFS(const FilesystemInfo& info)
{
    return canCloneOnFS(info.fsType);
}
bool canCloneOnFS(FilesystemType type)
{
    return s_clone_filesystems.contains(type);
}

/**
 * @brief if the Filesystem is reflink/clone capable and both paths are on the same device
 * 
 */
bool canClone(const QString& src, const QString& dst)
{
    auto srcVInfo = statFS(src);
    auto dstVInfo = statFS(dst);

    bool sameDevice = srcVInfo.rootPath == dstVInfo.rootPath;

    return sameDevice && canCloneOnFS(srcVInfo) && canCloneOnFS(dstVInfo);
}

/**
 * @brief reflink/clones a directory and it's contents from src to dest
 * @param offset subdirectory form src to copy to dest
 * @return if there was an error during the filecopy
 */
bool clone::operator()(const QString& offset, bool dryRun)
{

    if (!canClone(m_src.absolutePath(), m_dst.absolutePath())) {
        qWarning() << "Can not clone: not same device or not clone/reflink filesystem";
        qDebug() << "Source path:" << m_src.absolutePath();
        qDebug() << "Destination path:" << m_dst.absolutePath();
        emit cloneFailed(m_src.absolutePath(), m_dst.absolutePath());
        return false;
    }

    m_cloned = 0;  // reset counter

    auto src = PathCombine(m_src.absolutePath(), offset);
    auto dst = PathCombine(m_dst.absolutePath(), offset);

    std::error_code err;

    // Function that'll do the actual cloneing
    auto cloneFile = [&](QString src_path, QString relative_dst_path) {
        if (m_matcher && (m_matcher->matches(relative_dst_path) != m_whitelist))
            return;

        auto dst_path = PathCombine(dst, relative_dst_path);
        if (!dryRun) {
            ensureFilePathExists(dst_path);
            clone_file(src_path, dst_path, err);
        }
        if (err) {
            qWarning() << "Failed to clone files:" << QString::fromStdString(err.message());
            qDebug() << "Source file:" << src_path;
            qDebug() << "Destination file:" << dst_path;
        }
        m_cloned++;
        emit fileCloned(src_path, dst_path);
    };

    // We can't use copy_opts::recursive because we need to take into account the
    // blacklisted paths, so we iterate over the source directory, and if there's no blacklist
    // match, we copy the file.
    QDir src_dir(src);
    QDirIterator source_it(src, QDir::Filter::Files | QDir::Filter::Hidden, QDirIterator::Subdirectories);

    while (source_it.hasNext()) {
        auto src_path = source_it.next();
        auto relative_path = src_dir.relativeFilePath(src_path);

        cloneFile(src_path, relative_path);
    }

    // If the root src is not a directory, the previous iterator won't run.
    if (!fs::is_directory(StringUtils::toStdString(src)))
        cloneFile(src, "");

    return err.value() == 0;
}

/**
 * @brief clone/reflink file from src to dst
 * 
 */
bool clone_file(const QString& src, const QString& dst, std::error_code& ec)
{   
    auto src_path = StringUtils::toStdString(QDir::toNativeSeparators(QFileInfo(src).absoluteFilePath()));
    auto dst_path = StringUtils::toStdString(QDir::toNativeSeparators(QFileInfo(dst).absoluteFilePath()));

#if defined(Q_OS_WIN)
    FilesystemInfo srcinfo = statFS(src);
    if (srcinfo.fsType == FilesystemType::BTRFS) {
        FilesystemInfo dstinfo = statFS(dst);
        if (dstinfo.fsType != FilesystemType::BTRFS || (srcinfo.rootPath != dstinfo.rootPath)){
            qWarning() << "winbtrfs clone must be to the same device! src and dst root paths do not match.";
            qWarning() << "check out https://github.com/maharmstone/btrfs for btrfs support!";
            ec = std::make_error_code(std::errc::not_supported);
            return false;
        }

        qWarning() << "clone/reflink of btrfs on windows! assuming winbtrfs is in use and calling shellbtrfs.dll via rundll32.exe";

        if (!winbtrfs_clone(src_path, dst_path, ec))
            return false;

        // There is no return value from rundll32.exe so we must check if the file exsists ourselves

        QFileInfo dstInfo(dst);
        if (!dstInfo.exists() || !dstInfo.isFile() || dstInfo.isSymLink()) {
            // shellbtrfs.dll,ReflinkCopyW is curently broken https://github.com/maharmstone/btrfs/issues/556
            // lets try a little workaround
            // find the misnamed file
            qDebug() << dst << "is missing. ReflinkCopyW may still be broken, trying workaround.";
            QString badDst = QDir(dstInfo.absolutePath()).path() + dstInfo.fileName();
            qDebug() << "trying" << badDst;
            QFileInfo badDstInfo(badDst);
            if (badDstInfo.exists() && badDstInfo.isFile()) {
                qDebug() << badDst << "exists! moving it to the correct location.";
                if(!move(badDstInfo.absoluteFilePath(), dstInfo.absoluteFilePath())) {
                    qDebug() << "move from" << badDst << "to" << dst << "failed";
                    ec = std::make_error_code(std::errc::no_such_file_or_directory);
                    return false;
                }
            } else {
                // oof, clone failure?
                qWarning() << "clone/reflink on winbtrfs did not succeed: file" << dst << "does not appear to exsist";
                ec = std::make_error_code(std::errc::no_such_file_or_directory);
                return false;
            }  
        }

    } else if (srcinfo.fsType == FilesystemType::REFS) {
        qWarning() << "clone/reflink not yet supported on windows ReFS!";
        ec = std::make_error_code(std::errc::not_supported);
        return false;
    } else {
        qWarning() << "clone/reflink not supported on windows outside of winbtrfs or ReFS!";
        qWarning() << "check out https://github.com/maharmstone/btrfs for btrfs support!";
        ec = std::make_error_code(std::errc::not_supported);
        return false;
    }
#elif defined(Q_OS_LINUX)

    if(!linux_ficlone(src_path, dst_path, ec))
        return false;

#elif defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
   
    if(!macos_bsd_clonefile(src_path, dst_path, ec))
        return false;

#else
    qWarning() << "clone/reflink not supported! unknown OS";
    ec = std::make_error_code(std::errc::not_supported);
    return false;
#endif

    return true;
}

#if defined(Q_OS_WIN)
typedef void (__stdcall *f_ReflinkCopyW)(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow);

bool winbtrfs_clone(const std::wstring& src_path, const std::wstring& dst_path, std::error_code& ec)
{
    // https://github.com/maharmstone/btrfs
    QString cmdLine = QString("\"%1\" \"%2\"").arg(src_path, dst_path);

    std::wstring wstr = cmdLine.toStdWString(); // temp buffer to copy the data and avoid side effect of non const cast

    LPWSTR cmdLineWin = (wchar_t*)wstr.c_str();

    // https://github.com/maharmstone/btrfs/blob/9da54911dd6f3713a1c4c7be40338a3da126f4e6/src/shellext/contextmenu.cpp#L1609
    HINSTANCE shellbtrfsDLL = LoadLibrary(L"shellbtrfs.dll");

    if (shellbtrfsDLL == NULL) {
        ec = std::make_error_code(std::errc::not_supported);
        qWarning() << "cannot locate the shellbtrfs.dll file, reflink copy not supported";
        return false;
    }

    f_ReflinkCopyW ReflinkCopyW = (f_ReflinkCopyW)GetProcAddress(shellbtrfsDLL, "ReflinkCopyW");

    if (!ReflinkCopyW) {
        ec = std::make_error_code(std::errc::not_supported);
        qWarning() << "cannot locate the ReflinkCopyW function from shellbtrfs.dll, reflink copy not supported";
        return false;
    }

    qDebug() << "Calling ReflinkCopyW from shellbtrfs.dll with:" << cmdLine;

    ReflinkCopyW(0, 0, cmdLineWin, 1);

    FreeLibrary(shellbtrfsDLL);

    return true;
}

bool refs_clone(const std::wstring& src_path, const std::wstring& dst_path, std::error_code& ec)
{
#if defined(FSCTL_DUPLICATE_EXTENTS_TO_FILE)
    //https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_duplicate_extents_to_file
    //https://github.com/microsoft/CopyOnWrite/blob/main/lib/Windows/WindowsCopyOnWriteFilesystem.cs#L94
    std::wstring existingFile = src_path.c_str();
    std::wstring newLink = dst_path.c_str();

    
    HANDLE hExistingFile = CreateFile(src_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hExistingFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    HANDLE hNewFile = CreateFile(dst_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
    if (hNewFile == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hExistingFile);
        return false;
    }

    DWORD bytesReturned;

    // FIXME: ReFS requires that cloned regions reside on a disk cluster boundary.
    // FIXME: ERROR_BLOCK_TOO_MANY_REFERENCES can occure https://docs.microsoft.com/en-us/windows-server/storage/refs/block-cloning#functionality-restrictions-and-remarks
    BOOL result = DeviceIoControl(hExistingFile, FSCTL_DUPLICATE_EXTENTS_TO_FILE, hNewFile, sizeof(hNewFile), NULL, 0, &bytesReturned, NULL);

    CloseHandle(hNewFile);
    CloseHandle(hExistingFile);

    return (result != 0);
#else
    ec = std::make_error_code(std::errc::not_supported);
    qWarning() << "not built with refs support";
    return false;
#endif
}


#elif defined(Q_OS_LINUX)
bool linux_ficlone(const std::string& src_path, const std::string& dst_path, std::error_code& ec)
{
    // https://man7.org/linux/man-pages/man2/ioctl_ficlone.2.html

    int src_fd = open(src_path.c_str(), O_RDONLY);
    if(src_fd == -1) {
        qWarning() << "Failed to open file:" << src_path.c_str();
        qDebug() << "Error:" << strerror(errno);
        ec = std::make_error_code(static_cast<std::errc>(errno));
        return false;
    }
    int dst_fd = open(dst_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(dst_fd == -1) {
        qWarning() << "Failed to open file:" << dst_path.c_str();
        qDebug() << "Error:" << strerror(errno);
        ec = std::make_error_code(static_cast<std::errc>(errno));
        close(src_fd);
        return false;
    }
    // attempt to clone
    if(ioctl(dst_fd,  FICLONE, src_fd) == -1){
        qWarning() << "Failed to clone file:" << src_path.c_str() << "to" << dst_path.c_str();
        qDebug() << "Error:" << strerror(errno);
        ec = std::make_error_code(static_cast<std::errc>(errno));
        close(src_fd);
        close(dst_fd);
        return false;
    }
    if(close(src_fd)) {
        qWarning() << "Failed to close file:" << src_path.c_str();
        qDebug() << "Error:" << strerror(errno);
    }
    if(close(dst_fd)) {
        qWarning() << "Failed to close file:" << dst_path.c_str();
        qDebug() << "Error:" << strerror(errno);
    }
    return true;
}

#elif defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
bool macos_bsd_clonefile(const std::string& src_path, const std::string& dst_path, std::error_code& ec)
{
    // clonefile(const char * src, const char * dst, int flags);
    // https://www.manpagez.com/man/2/clonefile/

    qDebug() << "attempting file clone via clonefile" << src_path.c_str() << "to" << dst_path.c_str();
    if (clonefile(src_path.c_str(), dst_path.c_str(), 0) == -1) {
        qWarning() << "Failed to clone file:" << src_path.c_str() << "to" << dst_path.c_str();
        qDebug() << "Error:" << strerror(errno);
        ec = std::make_error_code(static_cast<std::errc>(errno));
        return false;
    }
    return true;
}
#endif

/**
 * @brief if the Filesystem is symlink capable 
 * 
 */
bool canLinkOnFS(const QString& path)
{
    FilesystemInfo info = statFS(path);
    return canLinkOnFS(info);
}
bool canLinkOnFS(const FilesystemInfo& info)
{
    return canLinkOnFS(info.fsType);
}
bool canLinkOnFS(FilesystemType type)
{
    return !s_non_link_filesystems.contains(type);
}
/**
 * @brief if the Filesystem is symlink capable on both ends
 * 
 */
bool canLink(const QString& src, const QString& dst)
{
    return  canLinkOnFS(src) && canLinkOnFS(dst);
}


}
