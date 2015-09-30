/* Copyright 2013-2015 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QString>
#include <QDir>

#include "multimc_util_export.h"

MULTIMC_UTIL_EXPORT QString PathCombine(QString path1, QString path2);
MULTIMC_UTIL_EXPORT QString PathCombine(QString path1, QString path2, QString path3);

MULTIMC_UTIL_EXPORT QString AbsolutePath(QString path);

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
MULTIMC_UTIL_EXPORT QString ResolveExecutable(QString path);

/**
 * Normalize path
 *
 * Any paths inside the current directory will be normalized to relative paths (to current)
 * Other paths will be made absolute
 *
 * Returns false if the path logic somehow filed (and normalizedPath in invalid)
 */
MULTIMC_UTIL_EXPORT QString NormalizePath(QString path);

MULTIMC_UTIL_EXPORT QString RemoveInvalidFilenameChars(QString string, QChar replaceWith = '-');

MULTIMC_UTIL_EXPORT QString DirNameFromString(QString string, QString inDir = ".");

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a file name and is ignored!
 */
MULTIMC_UTIL_EXPORT bool ensureFilePathExists(QString filenamepath);

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a folder name and is created!
 */
MULTIMC_UTIL_EXPORT bool ensureFolderPathExists(QString filenamepath);

/**
 * Copy a folder recursively
 */
MULTIMC_UTIL_EXPORT bool copyPath(const QString &src, const QString &dst, bool follow_symlinks = true);

/**
 * Delete a folder recursively
 */
MULTIMC_UTIL_EXPORT bool deletePath(QString path);

/// Opens the given file in the default application.
MULTIMC_UTIL_EXPORT void openFileInDefaultProgram(QString filename);

/// Opens the given directory in the default application.
MULTIMC_UTIL_EXPORT void openDirInDefaultProgram(QString dirpath, bool ensureExists = false);

/// Checks if the a given Path contains "!"
MULTIMC_UTIL_EXPORT bool checkProblemticPathJava(QDir folder);
