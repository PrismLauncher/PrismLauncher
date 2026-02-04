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
    close();
}

bool ArchiveWriter::open()
{
    if (m_filename.isEmpty()) {
        qCritical() << "Archive m_filename not set.";
        return false;
    }

    m_archive = archive_write_new();
    if (!m_archive) {
        qCritical() << "Archive not initialized.";
        return false;
    }

    auto format = m_format.toUtf8();
    archive_write_set_format_by_name(m_archive, format.constData());

    if (archive_write_set_options(m_archive, "hdrcharset=UTF-8") != ARCHIVE_OK) {
        qCritical() << "Failed to open archive file:" << m_filename << "-" << archive_error_string(m_archive);
        return false;
    }

    auto archiveNameW = m_filename.toStdWString();
    if (archive_write_open_filename_w(m_archive, archiveNameW.data()) != ARCHIVE_OK) {
        qCritical() << "Failed to open archive file:" << m_filename << "-" << archive_error_string(m_archive);
        return false;
    }

    return true;
}

bool ArchiveWriter::close()
{
    bool success = true;
    if (m_archive) {
        if (archive_write_close(m_archive) != ARCHIVE_OK) {
            qCritical() << "Failed to close archive" << m_filename << "-" << archive_error_string(m_archive);
            success = false;
        }
        if (archive_write_free(m_archive) != ARCHIVE_OK) {
            qCritical() << "Failed to free archive" << m_filename << "-" << archive_error_string(m_archive);
            success = false;
        }
        m_archive = nullptr;
    }
    return success;
}

bool ArchiveWriter::addFile(const QString& fileName, const QString& fileDest)
{
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists()) {
        qCritical() << "File does not exist:" << fileInfo.filePath();
        return false;
    }

    std::unique_ptr<archive_entry, void (*)(archive_entry*)> entry_ptr(archive_entry_new(), archive_entry_free);
    auto entry = entry_ptr.get();
    if (!entry) {
        qCritical() << "Failed to create archive entry";
        return false;
    }

    auto fileDestUtf8 = fileDest.toUtf8();
    archive_entry_set_pathname_utf8(entry, fileDestUtf8.constData());

#if defined Q_OS_WIN32
    {
        // Windows needs to use this method, thanks I hate it.

        auto widePath = fileInfo.absoluteFilePath().toStdWString();
        HANDLE file_handle = CreateFileW(widePath.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file_handle == INVALID_HANDLE_VALUE) {
            qCritical() << "Failed to stat file:" << fileInfo.filePath();
            return false;
        }

        BY_HANDLE_FILE_INFORMATION file_info;
        if (!GetFileInformationByHandle(file_handle, &file_info)) {
            qCritical() << "Failed to stat file:" << fileInfo.filePath();
            CloseHandle(file_handle);
            return false;
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
            qCritical() << "Failed to stat file:" << fileInfo.filePath();
            return false;
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
        qCritical() << "Unsupported file type:" << fileInfo.filePath();
        return false;
    }

    if (archive_write_header(m_archive, entry) != ARCHIVE_OK) {
        qCritical() << "Failed to write header for:" << fileDest << "-" << archive_error_string(m_archive);
        return false;
    }

    if (fileInfo.isFile() && !fileInfo.isSymLink()) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open file:" << fileInfo.filePath();
            return false;
        }

        constexpr qint64 chunkSize = 8192;
        QByteArray buffer;
        buffer.resize(chunkSize);

        while (!file.atEnd()) {
            auto bytesRead = file.read(buffer.data(), chunkSize);
            if (bytesRead < 0) {
                qCritical() << "Read error in file:" << fileInfo.filePath();
                return false;
            }

            if (archive_write_data(m_archive, buffer.constData(), bytesRead) < 0) {
                qCritical() << "Write error in archive for:" << fileDest;
                return false;
            }
        }
    }

    return true;
}

bool ArchiveWriter::addFile(const QString& fileDest, const QByteArray& data)
{
    std::unique_ptr<archive_entry, void (*)(archive_entry*)> entry_ptr(archive_entry_new(), archive_entry_free);
    auto entry = entry_ptr.get();
    if (!entry) {
        qCritical() << "Failed to create archive entry";
        return false;
    }

    auto fileDestUtf8 = fileDest.toUtf8();
    archive_entry_set_pathname_utf8(entry, fileDestUtf8.constData());
    archive_entry_set_perm(entry, 0644);

    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_size(entry, data.size());

    if (archive_write_header(m_archive, entry) != ARCHIVE_OK) {
        qCritical() << "Failed to write header for:" << fileDest << "-" << archive_error_string(m_archive);
        return false;
    }

    if (archive_write_data(m_archive, data.constData(), data.size()) < 0) {
        qCritical() << "Write error in archive for:" << fileDest << "-" << archive_error_string(m_archive);
        return false;
    }
    return true;
}

bool ArchiveWriter::addFile(ArchiveReader::File* f)
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
