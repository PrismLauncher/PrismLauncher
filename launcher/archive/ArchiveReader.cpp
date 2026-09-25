// SPDX-License-Identifier: GPL-3.0-only AND LicenseRef-PublicDomain
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
 *
 *  Additional note: Portions of this file are released into the public domain
 *  under LicenseRef-PublicDomain.
 */
#include "ArchiveReader.h"
#include <archive.h>
#include <archive_entry.h>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

namespace MMCZip {
QStringList ArchiveReader::getFiles()
{
    return m_fileNames;
}

Result<> ArchiveReader::collectFiles(bool onlyFiles)
{
    return parse([this, onlyFiles](File* f) {
        if (!onlyFiles || f->isFile()) {
            m_fileNames << f->filename();
        }
        return f->skip();
    });
}

using getPathFunc = std::function<const char*(archive_entry*)>;
static QString decodeLibArchivePath(archive_entry* entry, const getPathFunc& getUtf8Path, const getPathFunc& getPath)
{
    auto fileName = QString::fromUtf8(getUtf8Path(entry));
    if (fileName.isEmpty()) {
        fileName = QString::fromLocal8Bit(getPath(entry));
    }
    return fileName;
}

QString ArchiveReader::File::filename()
{
    return decodeLibArchivePath(m_entry, archive_entry_pathname_utf8, archive_entry_pathname);
}

Result<QByteArray> ArchiveReader::File::readAll()
{
    QByteArray data;
    const void* buff = nullptr;
    size_t size = 0;
    la_int64_t offset = 0;

    int status = 0;
    while ((status = archive_read_data_block(m_archive.get(), &buff, &size, &offset)) == ARCHIVE_OK) {
        data.append(static_cast<const char*>(buff), static_cast<qsizetype>(size));
    }
    if (status != ARCHIVE_EOF && status != ARCHIVE_OK) {
        return std::unexpected{QString("Could not read data block: %1").arg(error())};
    }
    return data;
}

QDateTime ArchiveReader::File::dateTime()
{
    auto mtime = archive_entry_mtime(m_entry);
    auto mtime_nsec = archive_entry_mtime_nsec(m_entry);
    auto dt = QDateTime::fromSecsSinceEpoch(mtime);
    return dt.addMSecs(mtime_nsec / 1e6);
}

int ArchiveReader::File::readNextHeader()
{
    return archive_read_next_header(m_archive.get(), &m_entry);
}

auto ArchiveReader::goToFile(const QString& filename) -> Result<std::unique_ptr<File>>
{
    auto f = std::make_unique<File>();
    auto* a = f->m_archive.get();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    auto fileName = m_archivePath.toStdWString();
    if (archive_read_open_filename_w(a, fileName.data(), m_blockSize) != ARCHIVE_OK) {
        return std::unexpected{QString("Failed to open archive file %1: %2").arg(m_archivePath, archive_error_string(a))};
    }

    while (f->readNextHeader() == ARCHIVE_OK) {
        if (f->filename() == filename) {
            return f;
        }
        TRY(f->skip())
    }

    archive_read_close(a);
    return std::unexpected{"File not found"};
}

Result<QByteArray> ArchiveReader::readFile(const QString& fileName)
{
    return goToFile(fileName).and_then([](const auto& v) -> Result<QByteArray> {
        return v->readAll();
    });
}

static int copy_data(struct archive* ar, struct archive* aw, bool notBlock = false)
{
    int r = 0;
    const void* buff = nullptr;
    size_t size = 0;
    la_int64_t offset = 0;

    for (;;) {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF) {
            return ARCHIVE_OK;
        }
        if (r < ARCHIVE_OK) {
            qCritical() << "Failed reading data block:" << archive_error_string(ar);
            return (r);
        }
        if (notBlock) {
            r = archive_write_data(aw, buff, size);
        } else {
            r = archive_write_data_block(aw, buff, size, offset);
        }
        if (r < ARCHIVE_OK) {
            qCritical() << "Failed writing data block:" << archive_error_string(aw);
            return (r);
        }
    }
}

static bool willEscapeRoot(const QDir& root, archive_entry* entry)
{
    auto entryPath = decodeLibArchivePath(entry, archive_entry_pathname_utf8, archive_entry_pathname);
    auto linkTarget = decodeLibArchivePath(entry, archive_entry_symlink_utf8, archive_entry_symlink);
    auto hardLink = decodeLibArchivePath(entry, archive_entry_hardlink_utf8, archive_entry_hardlink);

    if (entryPath.isEmpty() || (linkTarget.isEmpty() && hardLink.isEmpty())) {
        return false;
    }

    bool isHardLink = false;
    if (isHardLink = linkTarget.isEmpty(); isHardLink) {
        linkTarget = hardLink;
    }

    QString linkFullPath = root.filePath(entryPath);
    auto rootDir = QUrl::fromLocalFile(root.absolutePath());

    if (!rootDir.isParentOf(QUrl::fromLocalFile(linkFullPath))) {
        return true;
    }

    QDir linkDir = QFileInfo(linkFullPath).dir();
    if (!QDir::isAbsolutePath(linkTarget)) {
        linkTarget = (!isHardLink ? linkDir : root).filePath(linkTarget);
    }
    return !rootDir.isParentOf(QUrl::fromLocalFile(QDir::cleanPath(linkTarget)));
}

Result<> ArchiveReader::File::writeFile(archive* out, const QString& targetFileName, bool notBlock)
{
    return writeFile(out, targetFileName, {}, notBlock);
};

Result<> ArchiveReader::File::writeFile(archive* out, const QString& targetFileName, std::optional<QDir> root, bool notBlock)
{
    auto* entry = m_entry;
    std::unique_ptr<archive_entry, decltype(&archive_entry_free)> entryClone(nullptr, &archive_entry_free);
    if (!targetFileName.isEmpty()) {
        entryClone.reset(archive_entry_clone(m_entry));
        entry = entryClone.get();
        auto nameUtf8 = targetFileName.toUtf8();
        archive_entry_set_pathname_utf8(entry, nameUtf8.constData());

        if (root.has_value()) {
            auto hardLink = decodeLibArchivePath(entry, archive_entry_hardlink_utf8, archive_entry_hardlink);
            if (!hardLink.isEmpty() && !QDir::isAbsolutePath(hardLink)) {
                auto targetUtf8 = QDir::cleanPath(root->filePath(hardLink)).toUtf8();
                archive_entry_set_hardlink_utf8(entry, targetUtf8.constData());
            }
        }
    }
    if (root.has_value() && willEscapeRoot(root.value(), entry)) {
        return std::unexpected{"File is outside root"};
    }
    if (archive_write_header(out, entry) < ARCHIVE_OK) {
        return std::unexpected{QString("Failed to write header: %1").arg(archive_error_string(out))};
    }
    if (archive_entry_size(m_entry) > 0) {
        auto r = copy_data(m_archive.get(), out, notBlock);
        if (r < ARCHIVE_OK) {
            qCritical() << "Failed reading data block:" << archive_error_string(out);
        }
        if (r < ARCHIVE_WARN) {
            return std::unexpected{QString("Failed reading data block: %1").arg(archive_error_string(out))};
        }
    }
    auto r = archive_write_finish_entry(out);
    if (r < ARCHIVE_OK) {
        qCritical() << "Failed to finish writing entry:" << archive_error_string(out);
    }
    if (r < ARCHIVE_WARN) {
        return std::unexpected{QString("Failed to finish writing entry: %1").arg(archive_error_string(out))};
    }
    return {};
}

Result<> ArchiveReader::parse(const std::function<Result<bool>(File*)>& doStuff)
{
    auto f = std::make_unique<File>();
    auto* a = f->m_archive.get();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    auto fileName = m_archivePath.toStdWString();
    if (archive_read_open_filename_w(a, fileName.data(), m_blockSize) != ARCHIVE_OK) {
        return std::unexpected{QString("Failed to open archive file %1: %2").arg(m_archivePath, f->error())};
    }

    while (f->readNextHeader() == ARCHIVE_OK) {
        TRY_INTO(const bool shouldStop, doStuff(f.get()))
        if (shouldStop) {
            break;
        }
    }

    archive_read_close(a);
    return {};
}

Result<> ArchiveReader::parse(const std::function<Result<>(File*)>& doStuff)
{
    return parse([doStuff](File* f) -> Result<bool> {
        TRY(doStuff(f));
        return false;
    });
}

bool ArchiveReader::File::isFile()
{
    return (archive_entry_filetype(m_entry) & AE_IFMT) == AE_IFREG;
}
Result<> ArchiveReader::File::skip()
{
    if (archive_read_data_skip(m_archive.get()) < ARCHIVE_OK) {
        return std::unexpected{QString("Could not skip entry: %1").arg(error())};
    }
    return {};
}
const char* ArchiveReader::File::error()
{
    return archive_error_string(m_archive.get());
}
QString ArchiveReader::getZipName()
{
    return m_archivePath;
}

bool ArchiveReader::exists(const QString& filePath) const
{
    if (filePath == QLatin1String("/") || filePath.isEmpty()) {
        return true;
    }
    // Normalize input path (remove trailing slash, if any)
    QString normalizedPath = QDir::cleanPath(filePath);
    if (normalizedPath.startsWith('/')) {
        normalizedPath.remove(0, 1);
    }
    if (normalizedPath == QLatin1String(".")) {
        return true;
    }
    if (normalizedPath == QLatin1String("..")) {
        return false;  // root only
    }

    // Check for exact file match
    if (m_fileNames.contains(normalizedPath, Qt::CaseInsensitive)) {
        return true;
    }

    // Check for directory existence by seeing if any file starts with that path
    QString dirPath = normalizedPath + QLatin1Char('/');
    for (const QString& f : m_fileNames) {
        if (f.startsWith(dirPath, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

ArchiveReader::File::File() : m_archive(ArchivePtr(archive_read_new(), archive_read_free)) {}
}  // namespace MMCZip
