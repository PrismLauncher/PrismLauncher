// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <QDir>
#include <QFlags>
#include <QObject>

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

/// @brief Copies a directory and it's contents from src to dest
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

/**
 * Delete a folder recursively
 */
bool deletePath(QString path);

/**
 * Trash a folder / file
 */
bool trash(QString path, QString *pathInTrash);

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
