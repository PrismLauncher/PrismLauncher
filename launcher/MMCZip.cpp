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
bool mergeZipFiles(ArchiveWriter& into, QFileInfo from, QSet<QString>& contained, const FilterFunction& filter = nullptr)
{
    ArchiveReader r(from.absoluteFilePath());
    return r.parse([&into, &contained, &filter, from](ArchiveReader::File* f) {
        auto filename = f->filename();
        if (filter && !filter(filename)) {
            qDebug() << "Skipping file" << filename << "from" << from.fileName() << "- filtered";
            f->skip();
            return true;
        }
        if (contained.contains(filename)) {
            qDebug() << "Skipping already contained file" << filename << "from" << from.fileName();
            f->skip();
            return true;
        }
        contained.insert(filename);
        if (!into.addFile(f)) {
            qCritical() << "Failed to copy data of" << filename << "into the jar";
            return false;
        }
        return true;
    });
}

bool compressDirFiles(ArchiveWriter& zip, QString dir, QFileInfoList files)
{
    QDir directory(dir);
    if (!directory.exists())
        return false;

    for (auto e : files) {
        auto filePath = directory.relativeFilePath(e.absoluteFilePath());
        auto srcPath = e.absoluteFilePath();
        if (!zip.addFile(srcPath, filePath))
            return false;
    }

    return true;
}

// ours
bool createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod*>& mods)
{
    ArchiveWriter zipOut(targetJarPath);
    if (!zipOut.open()) {
        FS::deletePath(targetJarPath);
        qCritical() << "Failed to open the minecraft.jar for modding";
        return false;
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
            if (!mergeZipFiles(zipOut, mod->fileinfo(), addedFiles)) {
                zipOut.close();
                FS::deletePath(targetJarPath);
                qCritical() << "Failed to add" << mod->fileinfo().fileName() << "to the jar.";
                return false;
            }
        } else if (mod->type() == ResourceType::SINGLEFILE) {
            // FIXME: buggy - does not work with addedFiles
            auto filename = mod->fileinfo();
            if (!zipOut.addFile(filename.absoluteFilePath(), filename.fileName())) {
                zipOut.close();
                FS::deletePath(targetJarPath);
                qCritical() << "Failed to add" << mod->fileinfo().fileName() << "to the jar.";
                return false;
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

            if (!compressDirFiles(zipOut, parent_dir, files)) {
                zipOut.close();
                FS::deletePath(targetJarPath);
                qCritical() << "Failed to add" << mod->fileinfo().fileName() << "to the jar.";
                return false;
            }
            qDebug() << "Adding folder" << filename.fileName() << "from" << filename.absoluteFilePath();
        } else {
            // Make sure we do not continue launching when something is missing or undefined...
            zipOut.close();
            FS::deletePath(targetJarPath);
            qCritical() << "Failed to add unknown mod type" << mod->fileinfo().fileName() << "to the jar.";
            return false;
        }
    }

    if (!mergeZipFiles(zipOut, QFileInfo(sourceJarPath), addedFiles, [](const QString key) { return !key.contains("META-INF"); })) {
        zipOut.close();
        FS::deletePath(targetJarPath);
        qCritical() << "Failed to insert minecraft.jar contents.";
        return false;
    }

    // Recompress the jar
    if (!zipOut.close()) {
        FS::deletePath(targetJarPath);
        qCritical() << "Failed to finalize minecraft.jar!";
        return false;
    }
    return true;
}
#endif

// ours
std::optional<QStringList> extractSubDir(ArchiveReader* zip, const QString& subdir, const QString& target)
{
    auto target_top_dir = QUrl::fromLocalFile(target);

    QStringList extracted;

    qDebug() << "Extracting subdir" << subdir << "from" << zip->getZipName() << "to" << target;
    if (!zip->collectFiles()) {
        qWarning() << "Failed to enumerate files in archive";
        return std::nullopt;
    }
    if (zip->getFiles().isEmpty()) {
        qDebug() << "Extracting empty archives seems odd...";
        return extracted;
    }

    auto extPtr = ArchiveWriter::createDiskWriter();
    auto ext = extPtr.get();

    if (!zip->parse([&subdir, &target, &target_top_dir, ext, &extracted](ArchiveReader::File* f) {
            QString file_name = f->filename();
            file_name = FS::RemoveInvalidPathChars(file_name);
            if (!file_name.startsWith(subdir)) {
                f->skip();
                return true;
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
                qWarning() << "Extracting" << relative_file_name << "was cancelled, because it was effectively outside of the target path"
                           << target;
                return false;
            }
            if (!f->writeFile(ext, target_file_path)) {
                qWarning() << "Failed to extract file" << original_name << "to" << target_file_path;
                return false;
            }

            extracted.append(target_file_path);

            qDebug() << "Extracted file" << relative_file_name << "to" << target_file_path;
            return true;
        })) {
        qWarning() << "Failed to parse file" << zip->getZipName();
        FS::removeFiles(extracted);
        return std::nullopt;
    }

    return extracted;
}

// ours
std::optional<QStringList> extractDir(QString fileCompressed, QString dir)
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
std::optional<QStringList> extractDir(QString fileCompressed, QString subdir, QString dir)
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
bool extractFile(QString fileCompressed, QString file, QString target)
{
    // check if this is a minimum size empty zip file...
    QFileInfo fileInfo(fileCompressed);
    if (fileInfo.size() == 22) {
        return true;
    }
    ArchiveReader zip(fileCompressed);
    auto f = zip.goToFile(file);
    if (!f) {
        return false;
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
