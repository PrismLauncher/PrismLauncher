// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

#pragma once

#include "Exception.h"
#include "pathmatcher/IPathMatcher.h"

#include <system_error>

#include <QDir>
#include <QFlags>
#include <QObject>
#include <QThread>
#include <QLocalServer>

namespace FS {

class FileSystemException : public ::Exception {
   public:
    FileSystemException(const QString& message) : Exception(message) {}
};

/**
 * write data to a file safely
 */
void write(const QString& filename, const QByteArray& data);

/**
 * read data from a file safely\
 */
QByteArray read(const QString& filename);

/**
 * Update the last changed timestamp of an existing file
 */
bool updateTimestamp(const QString& filename);

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a file name and is ignored!
 */
bool ensureFilePathExists(QString filenamepath);

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a folder name and is created!
 */
bool ensureFolderPathExists(QString filenamepath);

/**
 * @brief Copies a directory and it's contents from src to dest
 */ 
class copy : public QObject {
    Q_OBJECT
   public:
    copy(const QString& src, const QString& dst, QObject* parent = nullptr) : QObject(parent)
    {
        m_src.setPath(src);
        m_dst.setPath(dst);
    }
    copy& followSymlinks(const bool follow)
    {
        m_followSymlinks = follow;
        return *this;
    }
    copy& matcher(const IPathMatcher* filter)
    {
        m_matcher = filter;
        return *this;
    }
    copy& whitelist(bool whitelist)
    {
        m_whitelist = whitelist;
        return *this;
    }

    bool operator()(bool dryRun = false) { return operator()(QString(), dryRun); }

    int totalCopied() { return m_copied; }

   signals:
    void fileCopied(const QString& relativeName);
    // TODO: maybe add a "shouldCopy" signal in the future?

   private:
    bool operator()(const QString& offset, bool dryRun = false);

   private:
    bool m_followSymlinks = true;
    const IPathMatcher* m_matcher = nullptr;
    bool m_whitelist = false;
    QDir m_src;
    QDir m_dst;
    int m_copied;
};

struct LinkPair {
    QString src;
    QString dst;
};

class ExternalLinkFileProcess : public QThread
{
    Q_OBJECT
   public:
    ExternalLinkFileProcess(QString server, QObject* parent = nullptr) : QThread(parent), m_server(server) {}

    void run() override {
        runLinkFile();
        emit processExited();
    }

   signals:
    void processExited();

   private:
    void runLinkFile();

    QString m_server;
};

/**
 * @brief links (a file / a directory and it's contents) from src to dest
 */ 
class create_link : public QObject {
    Q_OBJECT
   public:
    create_link(const QList<LinkPair> path_pairs, QObject* parent = nullptr) : QObject(parent)
    {
        m_path_pairs.append(path_pairs);
    }
    create_link(const QString& src, const QString& dst, QObject* parent = nullptr) : QObject(parent)
    {
        LinkPair pair = {src, dst};
        m_path_pairs.append(pair);
    }
    create_link& useHardLinks(const bool useHard)
    {
        m_useHardLinks = useHard;
        return *this;
    }
    create_link& matcher(const IPathMatcher* filter)
    {
        m_matcher = filter;
        return *this;
    }
    create_link& whitelist(bool whitelist)
    {
        m_whitelist = whitelist;
        return *this;
    }
    create_link& linkRecursively(bool recursive)
    {
        m_recursive = recursive;
        return *this;
    }
    create_link& debug(bool d)
    {
        m_debug = d;
        return *this;
    }

    std::error_code getOSError() {
        return m_os_err;
    }

    bool operator()(bool dryRun = false) { return operator()(QString(), dryRun); }

    bool runPrivlaged() { return runPrivlaged(QString()); }
    bool runPrivlaged(const QString& offset);

    int totalLinked() { return m_linked; }

   signals:
    void fileLinked(const QString& relativeName);
    void linkFailed(const QString& srcName, const QString& dstName, std::error_code err);
    void finishedPrivlaged();

   private:
    bool operator()(const QString& offset, bool dryRun = false);
    bool make_link(const QString& src_path, const QString& dst_path, const QString& offset, bool dryRun);

   private:
    bool m_useHardLinks = false;
    const IPathMatcher* m_matcher = nullptr;
    bool m_whitelist = false;
    bool m_recursive = true;

    QList<LinkPair> m_path_pairs;

    int m_linked;
    bool m_debug = false;
    std::error_code m_os_err;

    QLocalServer m_linkServer;
};

/**
 * @brief moves a file by renaming it
 * @param source source file path
 * @param dest destination filepath
 * 
 */
bool move(const QString&  source, const QString& dest);

/**
 * Delete a folder recursively
 */
bool deletePath(QString path);

/**
 * Trash a folder / file
 */
bool trash(QString path, QString *pathInTrash = nullptr);

QString PathCombine(const QString& path1, const QString& path2);
QString PathCombine(const QString& path1, const QString& path2, const QString& path3);
QString PathCombine(const QString& path1, const QString& path2, const QString& path3, const QString& path4);

QString AbsolutePath(QString path);

/**
 * Resolve an executable
 *
 * Will resolve:
 *   single executable (by name)
 *   relative path
 *   absolute path
 *
 * @return absolute path to executable or null string
 */
QString ResolveExecutable(QString path);

/**
 * Normalize path
 *
 * Any paths inside the current directory will be normalized to relative paths (to current)
 * Other paths will be made absolute
 *
 * Returns false if the path logic somehow filed (and normalizedPath in invalid)
 */
QString NormalizePath(QString path);

QString RemoveInvalidFilenameChars(QString string, QChar replaceWith = '-');

QString DirNameFromString(QString string, QString inDir = ".");

/// Checks if the a given Path contains "!"
bool checkProblemticPathJava(QDir folder);

// Get the Directory representing the User's Desktop
QString getDesktopDir();

// Overrides one folder with the contents of another, preserving items exclusive to the first folder
// Equivalent to doing QDir::rename, but allowing for overrides
bool overrideFolder(QString overwritten_path, QString override_path);

/**
 * Creates a shortcut to the specified target file at the specified destination path.
 */
bool createShortcut(QString destination, QString target, QStringList args, QString name, QString icon);
}
