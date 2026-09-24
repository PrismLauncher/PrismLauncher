// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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
 */
#include "ArchiveWriter.h"
#include <archive.h>
#include <archive_entry.h>
#include <sys/stat.h>

#include <QFile>
#include <QFileInfo>

#include <memory>
#include <system_error>
#include "StringUtils.h"

#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// clang-format off
#include <windows.h>
#include <fileapi.h>
// clang-format on
#endif

namespace MMCZip {

ArchiveWriter::ArchiveWriter(const QString& archiveName) : m_filename(archiveName) {}

ArchiveWriter::~ArchiveWriter()
{
    if (const auto result = close(); !result) {
        qWarning() << "ArchiveWriter automatic close failed:" << result.error();
    }
}

Result<> ArchiveWriter::open()
{
    if (m_filename.isEmpty()) {
        return std::unexpected{"'m_filename' not set."};
    }

    m_archive = archive_write_new();
    if (!m_archive) {
        return std::unexpected{QString("Failed to allocate writer object").arg(m_filename)};
    }

    auto format = m_format.toUtf8();
    if (archive_write_set_format_by_name(m_archive, format.constData()) != ARCHIVE_OK) {
        return std::unexpected{QString("Could not set format for archive %1: %2").arg(m_filename, archive_error_string(m_archive))};
    }

    if (archive_write_set_options(m_archive, "hdrcharset=UTF-8") != ARCHIVE_OK) {
        return std::unexpected{QString("Could not set charset for archive %1: %2").arg(m_filename, archive_error_string(m_archive))};
    }

    auto archiveNameW = m_filename.toStdWString();
    if (archive_write_open_filename_w(m_archive, archiveNameW.data()) != ARCHIVE_OK) {
        return std::unexpected{QString("Could not open archive %1 for writing: %2").arg(m_filename, archive_error_string(m_archive))};
    }

    return {};
}

Result<> ArchiveWriter::close()
{
    QStringList errors;
    if (m_archive) {
        if (archive_write_close(m_archive) != ARCHIVE_OK) {
            errors.append(QString("close: %1").arg(archive_error_string(m_archive)));
        }
        if (archive_write_free(m_archive) != ARCHIVE_OK) {
            errors.append(QString("free: %1").arg(archive_error_string(m_archive)));
        }
        m_archive = nullptr;
    }

    if (errors.isEmpty()) {
        return {};
    }
    return std::unexpected{QString("Could not close writer for archive %1: %2").arg(m_filename, errors.join("; "))};
}

Result<> ArchiveWriter::addFile(const QString& fileName, const QString& fileDest)
{
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists()) {
        return std::unexpected{QString("File does not exist: %1").arg(fileInfo.filePath())};
    }

    std::unique_ptr<archive_entry, void (*)(archive_entry*)> entry_ptr(archive_entry_new(), archive_entry_free);
    auto entry = entry_ptr.get();
    if (!entry) {
        return std::unexpected{"Could not allocate entry object"};
    }

    auto fileDestUtf8 = fileDest.toUtf8();
    archive_entry_set_pathname_utf8(entry, fileDestUtf8.constData());

#if defined Q_OS_WIN32
    {
        // Windows needs to use this method, thanks I hate it.

        auto widePath = fileInfo.absoluteFilePath().toStdWString();
        HANDLE file_handle = CreateFileW(widePath.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file_handle == INVALID_HANDLE_VALUE) {
            const auto err = std::error_code(static_cast<int>(GetLastError()), std::system_category());
            return std::unexpected{QString("Could not create file handle: %1").arg(err.message())};
        }

        BY_HANDLE_FILE_INFORMATION file_info;
        if (!GetFileInformationByHandle(file_handle, &file_info)) {
            const auto err = std::error_code(static_cast<int>(GetLastError()), std::system_category());
            CloseHandle(file_handle);
            return std::unexpected{QString("Could not get file information: %1").arg(err.message())};
        }

        archive_entry_copy_bhfi(entry, &file_info);
        CloseHandle(file_handle);
    }
#else
    {
        // this only works for multibyte encoded filenames if the local is properly set,
        // a wide character version doesn't seem to exist: here's hoping...

        QByteArray utf8 = fileInfo.absoluteFilePath().toUtf8();
        const char* cpath = utf8.constData();
        struct stat st;
        if (stat(cpath, &st) != 0) {
            const auto err = std::error_code(errno, std::system_category());
            return std::unexpected{ QString("Failed to stat file: %1").arg(StringUtils::fromStdString(err.message())) };
        }

        // This should handle the copying of most attributes
        archive_entry_copy_stat(entry, &st);
    }
#endif

    // However:
    // "The [filetype] constants used by stat(2) may have different numeric values from the corresponding [libarchive constants]."
    // - `archive_entry_stat(3)`
    if (fileInfo.isSymLink()) {
        archive_entry_set_filetype(entry, AE_IFLNK);

        // We also need to manually copy some attributes from the link itself, as `stat` above operates on its target
        auto target = fileInfo.symLinkTarget().toUtf8();
        archive_entry_set_symlink_utf8(entry, target.constData());
        archive_entry_set_size(entry, 0);
        archive_entry_set_perm(entry, fileInfo.permissions());
    } else if (fileInfo.isFile()) {
        archive_entry_set_filetype(entry, AE_IFREG);
    } else {
        return std::unexpected{QString("Unsupported file type: %1").arg(fileInfo.filePath())};
    }

    if (archive_write_header(m_archive, entry) != ARCHIVE_OK) {
        return std::unexpected{QString("Failed to write header: %1").arg(archive_error_string(m_archive))};
    }

    if (fileInfo.isFile() && !fileInfo.isSymLink()) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            return std::unexpected{QString("Failed to open file: %1").arg(file.errorString())};
        }

        constexpr qint64 chunkSize = 8192;
        QByteArray buffer;
        buffer.resize(chunkSize);

        while (!file.atEnd()) {
            auto bytesRead = file.read(buffer.data(), chunkSize);
            if (bytesRead < 0) {
                return std::unexpected{QString("Error reading file: %1").arg(file.errorString())};
            }

            if (archive_write_data(m_archive, buffer.constData(), bytesRead) < 0) {
                return std::unexpected{QString("Error writing data to archive: %1").arg(archive_error_string(m_archive))};
            }
        }
    }

    return {};
}

Result<> ArchiveWriter::addFile(const QString& fileDest, const QByteArray& data)
{
    std::unique_ptr<archive_entry, void (*)(archive_entry*)> entry_ptr(archive_entry_new(), archive_entry_free);
    auto entry = entry_ptr.get();
    if (!entry) {
        return std::unexpected{"Could not allocate entry object"};
    }

    auto fileDestUtf8 = fileDest.toUtf8();
    archive_entry_set_pathname_utf8(entry, fileDestUtf8.constData());
    archive_entry_set_perm(entry, 0644);

    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_size(entry, data.size());

    if (archive_write_header(m_archive, entry) != ARCHIVE_OK) {
        return std::unexpected{QString("Failed to write header: %1").arg(archive_error_string(m_archive))};
    }

    if (archive_write_data(m_archive, data.constData(), data.size()) < 0) {
        return std::unexpected{QString("Error writing data to archive: %1").arg(archive_error_string(m_archive))};
    }
    return {};
}

Result<> ArchiveWriter::addFile(ArchiveReader::File* f)
{
    return f->writeFile(m_archive, "", true);
}

std::unique_ptr<archive, void (*)(archive*)> ArchiveWriter::createDiskWriter()
{
    int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS |
                ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_SECURE_SYMLINKS;

    std::unique_ptr<archive, void (*)(archive*)> extPtr(archive_write_disk_new(), [](archive* a) {
        if (a) {
            archive_write_close(a);
            archive_write_free(a);
        }
    });

    archive* ext = extPtr.get();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

    return extPtr;
}
}  // namespace MMCZip
