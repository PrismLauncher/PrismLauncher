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

#include "libutil_config.h"

LIBUTIL_EXPORT QString PathCombine(QString path1, QString path2);
LIBUTIL_EXPORT QString PathCombine(QString path1, QString path2, QString path3);

LIBUTIL_EXPORT QString AbsolutePath(QString path);

/**
 * Normalize path
 *
 * Any paths inside the current directory will be normalized to relative paths (to current)
 * Other paths will be made absolute
 *
 * Returns false if the path logic somehow filed (and normalizedPath in invalid)
 */
QString NormalizePath(QString path);

LIBUTIL_EXPORT QString RemoveInvalidFilenameChars(QString string, QChar replaceWith = '-');

LIBUTIL_EXPORT QString DirNameFromString(QString string, QString inDir = ".");

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a file name and is ignored!
 */
LIBUTIL_EXPORT bool ensureFilePathExists(QString filenamepath);

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a folder name and is created!
 */
LIBUTIL_EXPORT bool ensureFolderPathExists(QString filenamepath);

LIBUTIL_EXPORT bool copyPath(QString src, QString dst);

/// Opens the given file in the default application.
LIBUTIL_EXPORT void openFileInDefaultProgram(QString filename);

/// Opens the given directory in the default application.
LIBUTIL_EXPORT void openDirInDefaultProgram(QString dirpath, bool ensureExists = false);

/// Checks if the a given Path contains "!"
LIBUTIL_EXPORT bool checkProblemticPathJava(QDir folder);
