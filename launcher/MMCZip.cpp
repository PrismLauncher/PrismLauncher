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
#include <quazip/quazip.h>
#include <quazip/quazipdir.h>
#include <quazip/quazipfile.h>
#include "FileSystem.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>

#if defined(LAUNCHER_APPLICATION)
#include <QtConcurrentRun>
#endif

namespace MMCZip {
// ours
bool mergeZipFiles(QuaZip* into, QFileInfo from, QSet<QString>& contained, const FilterFunction& filter)
{
    QuaZip modZip(from.filePath());
    modZip.open(QuaZip::mdUnzip);

    QuaZipFile fileInsideMod(&modZip);
    QuaZipFile zipOutFile(into);
    for (bool more = modZip.goToFirstFile(); more; more = modZip.goToNextFile()) {
        QString filename = modZip.getCurrentFileName();
        if (filter && !filter(filename)) {
            qDebug() << "Skipping file " << filename << " from " << from.fileName() << " - filtered";
            continue;
        }
        if (contained.contains(filename)) {
            qDebug() << "Skipping already contained file " << filename << " from " << from.fileName();
            continue;
        }
        contained.insert(filename);

        if (!fileInsideMod.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open " << filename << " from " << from.fileName();
            return false;
        }

        QuaZipNewInfo info_out(fileInsideMod.getActualFileName());

        if (!zipOutFile.open(QIODevice::WriteOnly, info_out)) {
            qCritical() << "Failed to open " << filename << " in the jar";
            fileInsideMod.close();
            return false;
        }
        if (!JlCompress::copyData(fileInsideMod, zipOutFile)) {
            zipOutFile.close();
            fileInsideMod.close();
            qCritical() << "Failed to copy data of " << filename << " into the jar";
            return false;
        }
        zipOutFile.close();
        fileInsideMod.close();
    }
    return true;
}

bool compressDirFiles(QuaZip* zip, QString dir, QFileInfoList files, bool followSymlinks)
{
    QDir directory(dir);
    if (!directory.exists())
        return false;

    for (auto e : files) {
        auto filePath = directory.relativeFilePath(e.absoluteFilePath());
        auto srcPath = e.absoluteFilePath();
        if (followSymlinks) {
            if (e.isSymLink()) {
                srcPath = e.symLinkTarget();
            } else {
                srcPath = e.canonicalFilePath();
            }
        }
        if (!JlCompress::compressFile(zip, srcPath, filePath))
            return false;
    }

    return true;
}

bool compressDirFiles(QString fileCompressed, QString dir, QFileInfoList files, bool followSymlinks)
{
    QuaZip zip(fileCompressed);
    zip.setUtf8Enabled(true);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if (!zip.open(QuaZip::mdCreate)) {
        FS::deletePath(fileCompressed);
        return false;
    }

    auto result = compressDirFiles(&zip, dir, files, followSymlinks);

    zip.close();
    if (zip.getZipError() != 0) {
        FS::deletePath(fileCompressed);
        return false;
    }

    return result;
}

#if defined(LAUNCHER_APPLICATION)
// ours
bool createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod*>& mods)
{
    QuaZip zipOut(targetJarPath);
    zipOut.setUtf8Enabled(true);
    if (!zipOut.open(QuaZip::mdCreate)) {
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
            if (!mergeZipFiles(&zipOut, mod->fileinfo(), addedFiles)) {
                zipOut.close();
                FS::deletePath(targetJarPath);
                qCritical() << "Failed to add" << mod->fileinfo().fileName() << "to the jar.";
                return false;
            }
        } else if (mod->type() == ResourceType::SINGLEFILE) {
            // FIXME: buggy - does not work with addedFiles
            auto filename = mod->fileinfo();
            if (!JlCompress::compressFile(&zipOut, filename.absoluteFilePath(), filename.fileName())) {
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

            if (!compressDirFiles(&zipOut, parent_dir, files)) {
                zipOut.close();
                FS::deletePath(targetJarPath);
                qCritical() << "Failed to add" << mod->fileinfo().fileName() << "to the jar.";
                return false;
            }
            qDebug() << "Adding folder " << filename.fileName() << " from " << filename.absoluteFilePath();
        } else {
            // Make sure we do not continue launching when something is missing or undefined...
            zipOut.close();
            FS::deletePath(targetJarPath);
            qCritical() << "Failed to add unknown mod type" << mod->fileinfo().fileName() << "to the jar.";
            return false;
        }
    }

    if (!mergeZipFiles(&zipOut, QFileInfo(sourceJarPath), addedFiles, [](const QString key) { return !key.contains("META-INF"); })) {
        zipOut.close();
        FS::deletePath(targetJarPath);
        qCritical() << "Failed to insert minecraft.jar contents.";
        return false;
    }

    // Recompress the jar
    zipOut.close();
    if (zipOut.getZipError() != 0) {
        FS::deletePath(targetJarPath);
        qCritical() << "Failed to finalize minecraft.jar!";
        return false;
    }
    return true;
}
#endif

// ours
QString findFolderOfFileInZip(QuaZip* zip, const QString& what, const QStringList& ignore_paths, const QString& root)
{
    QuaZipDir rootDir(zip, root);
    for (auto&& fileName : rootDir.entryList(QDir::Files)) {
        if (fileName == what)
            return root;

        QCoreApplication::processEvents();
    }

    // Recurse the search to non-ignored subfolders
    for (auto&& fileName : rootDir.entryList(QDir::Dirs)) {
        if (ignore_paths.contains(fileName))
            continue;

        QString result = findFolderOfFileInZip(zip, what, ignore_paths, root + fileName);
        if (!result.isEmpty())
            return result;
    }

    return {};
}

// ours
bool findFilesInZip(QuaZip* zip, const QString& what, QStringList& result, const QString& root)
{
    QuaZipDir rootDir(zip, root);
    for (auto fileName : rootDir.entryList(QDir::Files)) {
        if (fileName == what) {
            result.append(root);
            return true;
        }
    }
    for (auto fileName : rootDir.entryList(QDir::Dirs)) {
        findFilesInZip(zip, what, result, root + fileName);
    }
    return !result.isEmpty();
}

// ours
std::optional<QStringList> extractSubDir(QuaZip* zip, const QString& subdir, const QString& target)
{
    auto target_top_dir = QUrl::fromLocalFile(target);

    QStringList extracted;

    qDebug() << "Extracting subdir" << subdir << "from" << zip->getZipName() << "to" << target;
    auto numEntries = zip->getEntriesCount();
    if (numEntries < 0) {
        qWarning() << "Failed to enumerate files in archive";
        return std::nullopt;
    } else if (numEntries == 0) {
        qDebug() << "Extracting empty archives seems odd...";
        return extracted;
    } else if (!zip->goToFirstFile()) {
        qWarning() << "Failed to seek to first file in zip";
        return std::nullopt;
    }

    do {
        QString file_name = zip->getCurrentFileName();
        file_name = FS::RemoveInvalidPathChars(file_name);
        if (!file_name.startsWith(subdir))
            continue;

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
            return std::nullopt;
        }

        if (!JlCompress::extractFile(zip, "", target_file_path)) {
            qWarning() << "Failed to extract file" << original_name << "to" << target_file_path;
            JlCompress::removeFile(extracted);
            return std::nullopt;
        }

        extracted.append(target_file_path);
        auto fileInfo = QFileInfo(target_file_path);
        if (fileInfo.isFile()) {
            auto permissions = fileInfo.permissions();
            auto maxPermisions = QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser |
                                 QFileDevice::Permission::ReadGroup | QFileDevice::Permission::ReadOther;
            auto minPermisions = QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;

            auto newPermisions = (permissions & maxPermisions) | minPermisions;
            if (newPermisions != permissions) {
                if (!QFile::setPermissions(target_file_path, newPermisions)) {
                    qWarning() << (QObject::tr("Could not fix permissions for %1").arg(target_file_path));
                }
            }
        } else if (fileInfo.isDir()) {
            // Ensure the folder has the minimal required permissions
            QFile::Permissions minimalPermissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup |
                                                    QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther;

            QFile::Permissions currentPermissions = fileInfo.permissions();
            if ((currentPermissions & minimalPermissions) != minimalPermissions) {
                if (!QFile::setPermissions(target_file_path, minimalPermissions)) {
                    qWarning() << (QObject::tr("Could not fix permissions for %1").arg(target_file_path));
                }
            }
        }
        qDebug() << "Extracted file" << relative_file_name << "to" << target_file_path;
    } while (zip->goToNextFile());

    return extracted;
}

// ours
bool extractRelFile(QuaZip* zip, const QString& file, const QString& target)
{
    return JlCompress::extractFile(zip, file, target);
}

// ours
std::optional<QStringList> extractDir(QString fileCompressed, QString dir)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip)) {
        // check if this is a minimum size empty zip file...
        QFileInfo fileInfo(fileCompressed);
        if (fileInfo.size() == 22) {
            return QStringList();
        }
        qWarning() << "Could not open archive for unzipping:" << fileCompressed << "Error:" << zip.getZipError();
        ;
        return std::nullopt;
    }
    return extractSubDir(&zip, "", dir);
}

// ours
std::optional<QStringList> extractDir(QString fileCompressed, QString subdir, QString dir)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip)) {
        // check if this is a minimum size empty zip file...
        QFileInfo fileInfo(fileCompressed);
        if (fileInfo.size() == 22) {
            return QStringList();
        }
        qWarning() << "Could not open archive for unzipping:" << fileCompressed << "Error:" << zip.getZipError();
        ;
        return std::nullopt;
    }
    return extractSubDir(&zip, subdir, dir);
}

// ours
bool extractFile(QString fileCompressed, QString file, QString target)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip)) {
        // check if this is a minimum size empty zip file...
        QFileInfo fileInfo(fileCompressed);
        if (fileInfo.size() == 22) {
            return true;
        }
        qWarning() << "Could not open archive for unzipping:" << fileCompressed << "Error:" << zip.getZipError();
        return false;
    }
    return extractRelFile(&zip, file, target);
}

bool collectFileListRecursively(const QString& rootDir, const QString& subDir, QFileInfoList* files, FilterFunction excludeFilter)
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
        QString relativeFilePath = rootDirectory.relativeFilePath(e.absoluteFilePath());
        if (excludeFilter && excludeFilter(relativeFilePath)) {
            qDebug() << "Skipping file " << relativeFilePath;
            continue;
        }

        files->append(e);  // we want the original paths for compressDirFiles
    }
    return true;
}

#if defined(LAUNCHER_APPLICATION)
void ExportToZipTask::executeTask()
{
    setStatus("Adding files...");
    setProgress(0, m_files.length());
    m_build_zip_future = QtConcurrent::run(QThreadPool::globalInstance(), [this]() { return exportZip(); });
    connect(&m_build_zip_watcher, &QFutureWatcher<ZipResult>::finished, this, &ExportToZipTask::finish);
    m_build_zip_watcher.setFuture(m_build_zip_future);
}

auto ExportToZipTask::exportZip() -> ZipResult
{
    if (!m_dir.exists()) {
        return ZipResult(tr("Folder doesn't exist"));
    }
    if (!m_output.isOpen() && !m_output.open(QuaZip::mdCreate)) {
        return ZipResult(tr("Could not create file"));
    }

    for (auto fileName : m_extra_files.keys()) {
        if (m_build_zip_future.isCanceled())
            return ZipResult();
        QuaZipFile indexFile(&m_output);
        if (!indexFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileName))) {
            return ZipResult(tr("Could not create:") + fileName);
        }
        indexFile.write(m_extra_files[fileName]);
    }

    for (const QFileInfo& file : m_files) {
        if (m_build_zip_future.isCanceled())
            return ZipResult();

        auto absolute = file.absoluteFilePath();
        auto relative = m_dir.relativeFilePath(absolute);
        setStatus("Compressing: " + relative);
        setProgress(m_progress + 1, m_progressTotal);
        if (m_follow_symlinks) {
            if (file.isSymLink())
                absolute = file.symLinkTarget();
            else
                absolute = file.canonicalFilePath();
        }

        if (!m_exclude_files.contains(relative) && !JlCompress::compressFile(&m_output, absolute, m_destination_prefix + relative)) {
            return ZipResult(tr("Could not read and compress %1").arg(relative));
        }
    }

    m_output.close();
    if (m_output.getZipError() != 0) {
        return ZipResult(tr("A zip error occurred"));
    }
    return ZipResult();
}

void ExportToZipTask::finish()
{
    if (m_build_zip_future.isCanceled()) {
        FS::deletePath(m_output_path);
        emitAborted();
    } else if (auto result = m_build_zip_future.result(); result.has_value()) {
        FS::deletePath(m_output_path);
        emitFailed(result.value());
    } else {
        emitSucceeded();
    }
}

bool ExportToZipTask::abort()
{
    if (m_build_zip_future.isRunning()) {
        m_build_zip_future.cancel();
        // NOTE: Here we don't do `emitAborted()` because it will be done when `m_build_zip_future` actually cancels, which may not occur
        // immediately.
        return true;
    }
    return false;
}

void ExtractZipTask::executeTask()
{
    if (!m_input->isOpen() && !m_input->open(QuaZip::mdUnzip)) {
        emitFailed(tr("Unable to open supplied zip file."));
        return;
    }
    m_zip_future = QtConcurrent::run(QThreadPool::globalInstance(), [this]() { return extractZip(); });
    connect(&m_zip_watcher, &QFutureWatcher<ZipResult>::finished, this, &ExtractZipTask::finish);
    m_zip_watcher.setFuture(m_zip_future);
}

auto ExtractZipTask::extractZip() -> ZipResult
{
    auto target = m_output_dir.absolutePath();
    auto target_top_dir = QUrl::fromLocalFile(target);

    QStringList extracted;

    qDebug() << "Extracting subdir" << m_subdirectory << "from" << m_input->getZipName() << "to" << target;
    auto numEntries = m_input->getEntriesCount();
    if (numEntries < 0) {
        return ZipResult(tr("Failed to enumerate files in archive"));
    }
    if (numEntries == 0) {
        logWarning(tr("Extracting empty archives seems odd..."));
        return ZipResult();
    }
    if (!m_input->goToFirstFile()) {
        return ZipResult(tr("Failed to seek to first file in zip"));
    }

    setStatus("Extracting files...");
    setProgress(0, numEntries);
    do {
        if (m_zip_future.isCanceled())
            return ZipResult();
        setProgress(m_progress + 1, m_progressTotal);
        QString file_name = m_input->getCurrentFileName();
        if (!file_name.startsWith(m_subdirectory))
            continue;

        auto relative_file_name = QDir::fromNativeSeparators(file_name.mid(m_subdirectory.size()));
        auto original_name = relative_file_name;
        setStatus("Unziping: " + relative_file_name);

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
            return ZipResult(tr("Extracting %1 was cancelled, because it was effectively outside of the target path %2")
                                 .arg(relative_file_name, target));
        }

        if (!JlCompress::extractFile(m_input.get(), "", target_file_path)) {
            JlCompress::removeFile(extracted);
            return ZipResult(tr("Failed to extract file %1 to %2").arg(original_name, target_file_path));
        }

        extracted.append(target_file_path);
        auto fileInfo = QFileInfo(target_file_path);
        if (fileInfo.isFile()) {
            auto permissions = fileInfo.permissions();
            auto maxPermisions = QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser |
                                 QFileDevice::Permission::ReadGroup | QFileDevice::Permission::ReadOther;
            auto minPermisions = QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;

            auto newPermisions = (permissions & maxPermisions) | minPermisions;
            if (newPermisions != permissions) {
                if (!QFile::setPermissions(target_file_path, newPermisions)) {
                    logWarning(tr("Could not fix permissions for %1").arg(target_file_path));
                }
            }
        } else if (fileInfo.isDir()) {
            // Ensure the folder has the minimal required permissions
            QFile::Permissions minimalPermissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup |
                                                    QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther;

            QFile::Permissions currentPermissions = fileInfo.permissions();
            if ((currentPermissions & minimalPermissions) != minimalPermissions) {
                if (!QFile::setPermissions(target_file_path, minimalPermissions)) {
                    logWarning(tr("Could not fix permissions for %1").arg(target_file_path));
                }
            }
        }

        qDebug() << "Extracted file" << relative_file_name << "to" << target_file_path;
    } while (m_input->goToNextFile());

    return ZipResult();
}

void ExtractZipTask::finish()
{
    if (m_zip_future.isCanceled()) {
        emitAborted();
    } else if (auto result = m_zip_future.result(); result.has_value()) {
        emitFailed(result.value());
    } else {
        emitSucceeded();
    }
}

bool ExtractZipTask::abort()
{
    if (m_zip_future.isRunning()) {
        m_zip_future.cancel();
        // NOTE: Here we don't do `emitAborted()` because it will be done when `m_build_zip_future` actually cancels, which may not occur
        // immediately.
        return true;
    }
    return false;
}

#endif
}  // namespace MMCZip
