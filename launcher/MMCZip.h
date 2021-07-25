/* Copyright 2013-2021 MultiMC Contributors
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
#include <QFileInfo>
#include <QSet>
#include "minecraft/mod/Mod.h"
#include <functional>

#include <JlCompress.h>
#include <nonstd/optional>

namespace MMCZip
{

    /**
     * Merge two zip files, using a filter function
     */
    bool mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained,
                                            const JlCompress::FilterFunction filter = nullptr);

    /**
     * take a source jar, add mods to it, resulting in target jar
     */
    bool createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod>& mods);

    /**
     * Find a single file in archive by file name (not path)
     *
     * \return the path prefix where the file is
     */
    QString findFolderOfFileInZip(QuaZip * zip, const QString & what, const QString &root = QString(""));

    /**
     * Find a multiple files of the same name in archive by file name
     * If a file is found in a path, no deeper paths are searched
     *
     * \return true if anything was found
     */
    bool findFilesInZip(QuaZip * zip, const QString & what, QStringList & result, const QString &root = QString());

    /**
     * Extract a subdirectory from an archive
     */
    nonstd::optional<QStringList> extractSubDir(QuaZip *zip, const QString & subdir, const QString &target);

    bool extractRelFile(QuaZip *zip, const QString & file, const QString &target);

    /**
     * Extract a whole archive.
     *
     * \param fileCompressed The name of the archive.
     * \param dir The directory to extract to, the current directory if left empty.
     * \return The list of the full paths of the files extracted, empty on failure.
     */
    nonstd::optional<QStringList> extractDir(QString fileCompressed, QString dir);

    /**
     * Extract a subdirectory from an archive
     *
     * \param fileCompressed The name of the archive.
     * \param subdir The directory within the archive to extract
     * \param dir The directory to extract to, the current directory if left empty.
     * \return The list of the full paths of the files extracted, empty on failure.
     */
    nonstd::optional<QStringList> extractDir(QString fileCompressed, QString subdir, QString dir);

    /**
     * Extract a single file from an archive into a directory
     *
     * \param fileCompressed The name of the archive.
     * \param file The file within the archive to extract
     * \param dir The directory to extract to, the current directory if left empty.
     * \return true for success or false for failure
     */
    bool extractFile(QString fileCompressed, QString file, QString dir);

}
