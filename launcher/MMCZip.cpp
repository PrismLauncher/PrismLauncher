// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2023-2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include "MMCZip.h"
#include <archive.h>
#include "FileSystem.h"
#include "archive/ArchiveReader.h"
#include "archive/ArchiveWriter.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>
#include <memory>

namespace MMCZip {
// ours
using FilterFunction = std::function<bool(const QString&)>;
#if defined(LAUNCHER_APPLICATION)
Result<> mergeZipFiles(ArchiveWriter& into, QFileInfo from, QSet<QString>& contained, const FilterFunction& filter = nullptr)
{
    ArchiveReader r(from.absoluteFilePath());
    return r.parse([&into, &contained, &filter, from](ArchiveReader::File* f) {
        auto filename = f->filename();
        if (filter && !filter(filename)) {
            qDebug() << "Skipping file" << filename << "from" << from.fileName() << "- filtered";
            return f->skip();
        }
        if (contained.contains(filename)) {
            qDebug() << "Skipping already contained file" << filename << "from" << from.fileName();
            return f->skip();
        }
        contained.insert(filename);
        return into.addFile(f);
    });
}

Result<> compressDirFiles(ArchiveWriter& zip, QString dir, QFileInfoList files)
{
    QDir directory(dir);
    if (!directory.exists())
        return std::unexpected{ "Directory does not exist" };

    for (auto e : files) {
        auto filePath = directory.relativeFilePath(e.absoluteFilePath());
        auto srcPath = e.absoluteFilePath();
        TRY(zip.addFile(srcPath, filePath))
    }

    return {};
}

// ours
Result<> createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod*>& mods)
{
    ArchiveWriter zipOut(targetJarPath);
    if (const auto result = zipOut.open(); !result) {
        FS::deletePath(targetJarPath);
        return std::unexpected{ QString("Could not open minecraft.jar for modding: %1").arg(result.error()) };
    }
    // Files already added to the jar.
    // These files will be skipped.
    QSet<QString> addedFiles;

    // Modify the jar
    // This needs to be done in reverse-order to ensure we respect the loading order of components
    for (auto i = mods.crbegin(); i != mods.crend(); i++) {
        const auto* mod = *i;
        // do not merge disabled mods.
        if (!mod->enabled())
            continue;
        if (mod->type() == ResourceType::ZIPFILE) {
            if (const auto result = mergeZipFiles(zipOut, mod->fileinfo(), addedFiles); !result) {
                TRY_OR_LOG(zipOut.close());
                FS::deletePath(targetJarPath);
                return std::unexpected{ QString("Failed to add %1 to the jar: %2").arg(mod->fileinfo().fileName(), result.error()) };
            }
        } else if (mod->type() == ResourceType::SINGLEFILE) {
            // FIXME: buggy - does not work with addedFiles
            auto filename = mod->fileinfo();
            if (const auto result = zipOut.addFile(filename.absoluteFilePath(), filename.fileName()); !result) {
                TRY_OR_LOG(zipOut.close());
                FS::deletePath(targetJarPath);
                return std::unexpected{ QString("Failed to add %1 to the jar: %2").arg(mod->fileinfo().fileName(), result.error()) };
            }
            addedFiles.insert(filename.fileName());
        } else if (mod->type() == ResourceType::FOLDER) {
            // untested, but seems to be unused / not possible to reach
            // FIXME: buggy - does not work with addedFiles
            auto filename = mod->fileinfo();
            QString what_to_zip = filename.absoluteFilePath();
            QDir dir(what_to_zip);
            dir.cdUp();
            QString parent_dir = dir.absolutePath();
            auto files = QFileInfoList();
            collectFileListRecursively(what_to_zip, nullptr, &files, nullptr);

            for (auto e : files) {
                if (addedFiles.contains(e.filePath()))
                    files.removeAll(e);
            }

            if (const auto result = compressDirFiles(zipOut, parent_dir, files); !result) {
                TRY_OR_LOG(zipOut.close());
                FS::deletePath(targetJarPath);
                return std::unexpected{ QString("Failed to add %1 to the jar: %2").arg(mod->fileinfo().fileName(), result.error()) };
            }
            qDebug() << "Adding folder" << filename.fileName() << "from" << filename.absoluteFilePath();
        } else {
            // Make sure we do not continue launching when something is missing or undefined...
            TRY_OR_LOG(zipOut.close());
            FS::deletePath(targetJarPath);
            return std::unexpected{ QString("Failed to add unknown mod type %1 to the jar.").arg(mod->fileinfo().fileName()) };
        }
    }

    if (const auto result =
            mergeZipFiles(zipOut, QFileInfo(sourceJarPath), addedFiles, [](const QString key) { return !key.contains("META-INF"); });
        !result) {
        TRY_OR_LOG(zipOut.close());
        FS::deletePath(targetJarPath);
        return std::unexpected{ QString("Failed to insert minecraft.jar contents: %1").arg(result.error()) };
    }

    // Recompress the jar
    if (const auto result = zipOut.close(); !result) {
        FS::deletePath(targetJarPath);
        return std::unexpected{ QString("Failed to finalize minecraft.jar: %1").arg(result.error()) };
    }
    return {};
}
#endif

// ours
Result<QStringList> extractSubDir(ArchiveReader* zip, const QString& subdir, const QString& target)
{
    auto target_top_dir = QUrl::fromLocalFile(target);

    QStringList extracted;

    qDebug() << "Extracting subdir" << subdir << "from" << zip->getZipName() << "to" << target;
    TRY(zip->collectFiles())

    if (zip->getFiles().isEmpty()) {
        qDebug() << "Extracting empty archives seems odd...";
        return extracted;
    }

    auto extPtr = ArchiveWriter::createDiskWriter();
    auto ext = extPtr.get();

    if (const auto result = zip->parse([&subdir, &target, &target_top_dir, ext, &extracted](ArchiveReader::File* f) -> Result<> {
            QString file_name = f->filename();
            file_name = FS::RemoveInvalidPathChars(file_name);
            if (!file_name.startsWith(subdir)) {
                return f->skip();
            }

            auto relative_file_name = QDir::fromNativeSeparators(file_name.mid(subdir.size()));
            auto original_name = relative_file_name;

            // Fix subdirs/files ending with a / getting transformed into absolute paths
            if (relative_file_name.startsWith('/'))
                relative_file_name = relative_file_name.mid(1);

            // Fix weird "folders with a single file get squashed" thing
            QString sub_path;
            if (relative_file_name.contains('/') && !relative_file_name.endsWith('/')) {
                sub_path = relative_file_name.section('/', 0, -2) + '/';
                FS::ensureFolderPathExists(FS::PathCombine(target, sub_path));

                relative_file_name = relative_file_name.split('/').last();
            }
            QString target_file_path;
            if (relative_file_name.isEmpty()) {
                target_file_path = target + '/';
            } else {
                target_file_path = FS::PathCombine(target_top_dir.toLocalFile(), sub_path, relative_file_name);
                if (relative_file_name.endsWith('/') && !target_file_path.endsWith('/'))
                    target_file_path += '/';
            }

            if (!target_top_dir.isParentOf(QUrl::fromLocalFile(target_file_path))) {
                return std::unexpected{ QString("Extracting %1 was cancelled, because it was effectively outside of the target path %2")
                                            .arg(relative_file_name, target) };
            }

            TRY(f->writeFile(ext, target_file_path, target))
            extracted.append(target_file_path);

            qDebug() << "Extracted file" << relative_file_name << "to" << target_file_path;
            return {};
        });
        !result) {
        FS::removeFiles(extracted);
        return std::unexpected{ result.error() };
    }

    return extracted;
}

// ours
Result<QStringList> extractDir(QString fileCompressed, QString dir)
{
    // check if this is a minimum size empty zip file...
    QFileInfo fileInfo(fileCompressed);
    if (fileInfo.size() == 22) {
        return QStringList();
    }
    ArchiveReader zip(fileCompressed);
    return extractSubDir(&zip, "", dir);
}

// ours
Result<QStringList> extractDir(QString fileCompressed, QString subdir, QString dir)
{
    // check if this is a minimum size empty zip file...
    QFileInfo fileInfo(fileCompressed);
    if (fileInfo.size() == 22) {
        return QStringList();
    }
    ArchiveReader zip(fileCompressed);
    return extractSubDir(&zip, subdir, dir);
}

// ours
Result<> extractFile(QString fileCompressed, QString file, QString target)
{
    // check if this is a minimum size empty zip file...
    QFileInfo fileInfo(fileCompressed);
    if (fileInfo.size() == 22) {
        return {};
    }
    ArchiveReader zip(fileCompressed);
    TRY_INTO(const auto& f, zip.goToFile(file))
    if (!f) {
        return std::unexpected{ "File not found" };
    }
    auto extPtr = ArchiveWriter::createDiskWriter();
    auto ext = extPtr.get();

    return f->writeFile(ext, target);
}

bool collectFileListRecursively(const QString& rootDir, const QString& subDir, QFileInfoList* files, FilterFileFunction excludeFilter)
{
    QDir rootDirectory(rootDir);
    if (!rootDirectory.exists())
        return false;

    QDir directory;
    if (subDir == nullptr)
        directory = rootDirectory;
    else
        directory = QDir(subDir);

    if (!directory.exists())
        return false;  // shouldn't ever happen

    // recurse directories
    QFileInfoList entries = directory.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
    for (const auto& e : entries) {
        if (!collectFileListRecursively(rootDir, e.filePath(), files, excludeFilter))
            return false;
    }

    // collect files
    entries = directory.entryInfoList(QDir::Files);
    for (const auto& e : entries) {
        if (excludeFilter && excludeFilter(e)) {
            QString relativeFilePath = rootDirectory.relativeFilePath(e.absoluteFilePath());
            qDebug() << "Skipping file" << relativeFilePath;
            continue;
        }

        files->append(e);  // we want the original paths for compressDirFiles
    }
    return true;
}
}  // namespace MMCZip
