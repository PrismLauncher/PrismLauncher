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
#include <memory>

namespace MMCZip {
QStringList ArchiveReader::getFiles()
{
    return m_fileNames;
}

bool ArchiveReader::collectFiles(bool onlyFiles)
{
    return parse([this, onlyFiles](File* f) {
        if (!onlyFiles || f->isFile())
            m_fileNames << f->filename();
        return f->skip();
    });
}

QString ArchiveReader::File::filename()
{
    return QString::fromUtf8(archive_entry_pathname_utf8(m_entry));
}

QByteArray ArchiveReader::File::readAll(int* outStatus)
{
    QByteArray data;
    const void* buff;
    size_t size;
    la_int64_t offset;

    int status;
    while ((status = archive_read_data_block(m_archive.get(), &buff, &size, &offset)) == ARCHIVE_OK) {
        data.append(static_cast<const char*>(buff), static_cast<qsizetype>(size));
    }
    if (status != ARCHIVE_EOF && status != ARCHIVE_OK) {
        qWarning() << "libarchive read error:" << archive_error_string(m_archive.get());
    }
    if (outStatus) {
        *outStatus = status;
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

auto ArchiveReader::goToFile(QString filename) -> std::unique_ptr<File>
{
    auto f = std::make_unique<File>();
    auto a = f->m_archive.get();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    auto fileName = m_archivePath.toStdWString();
    if (archive_read_open_filename_w(a, fileName.data(), m_blockSize) != ARCHIVE_OK) {
        qCritical() << "Failed to open archive file:" << m_archivePath << "-" << archive_error_string(a);
        return nullptr;
    }

    while (f->readNextHeader() == ARCHIVE_OK) {
        if (f->filename() == filename) {
            return f;
        }
        f->skip();
    }

    archive_read_close(a);
    return nullptr;
}

static int copy_data(struct archive* ar, struct archive* aw, bool notBlock = false)
{
    int r;
    const void* buff;
    size_t size;
    la_int64_t offset;

    for (;;) {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
            return (ARCHIVE_OK);
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

bool ArchiveReader::File::writeFile(archive* out, QString targetFileName, bool notBlock)
{
    auto entry = m_entry;
    std::unique_ptr<archive_entry, decltype(&archive_entry_free)> entryClone(nullptr, &archive_entry_free);
    if (!targetFileName.isEmpty()) {
        entryClone.reset(archive_entry_clone(m_entry));
        entry = entryClone.get();
        auto nameUtf8 = targetFileName.toUtf8();
        archive_entry_set_pathname_utf8(entry, nameUtf8.constData());
    }
    if (archive_write_header(out, entry) < ARCHIVE_OK) {
        qCritical() << "Failed to write header to entry:" << filename() << "-" << archive_error_string(out);
        return false;
    } else if (archive_entry_size(m_entry) > 0) {
        auto r = copy_data(m_archive.get(), out, notBlock);
        if (r < ARCHIVE_OK)
            qCritical() << "Failed reading data block:" << archive_error_string(out);
        if (r < ARCHIVE_WARN)
            return false;
    }
    auto r = archive_write_finish_entry(out);
    if (r < ARCHIVE_OK)
        qCritical() << "Failed to finish writing entry:" << archive_error_string(out);
    return (r >= ARCHIVE_WARN);
}

bool ArchiveReader::parse(std::function<bool(File*, bool&)> doStuff)
{
    auto f = std::make_unique<File>();
    auto a = f->m_archive.get();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    auto fileName = m_archivePath.toStdWString();
    if (archive_read_open_filename_w(a, fileName.data(), m_blockSize) != ARCHIVE_OK) {
        qCritical() << "Failed to open archive file:" << m_archivePath << "-" << f->error();
        return false;
    }

    bool breakControl = false;
    while (f->readNextHeader() == ARCHIVE_OK) {
        if (f && !doStuff(f.get(), breakControl)) {
            qCritical() << "Failed to parse file:" << f->filename() << "-" << f->error();
            return false;
        }
        if (breakControl) {
            break;
        }
    }

    archive_read_close(a);
    return true;
}

bool ArchiveReader::parse(std::function<bool(File*)> doStuff)
{
    return parse([doStuff](File* f, bool&) { return doStuff(f); });
}

bool ArchiveReader::File::isFile()
{
    return (archive_entry_filetype(m_entry) & AE_IFMT) == AE_IFREG;
}
bool ArchiveReader::File::skip()
{
    return archive_read_data_skip(m_archive.get()) == ARCHIVE_OK;
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
    if (filePath == QLatin1String("/") || filePath.isEmpty())
        return true;
    // Normalize input path (remove trailing slash, if any)
    QString normalizedPath = QDir::cleanPath(filePath);
    if (normalizedPath.startsWith('/'))
        normalizedPath.remove(0, 1);
    if (normalizedPath == QLatin1String("."))
        return true;
    if (normalizedPath == QLatin1String(".."))
        return false;  // root only

    // Check for exact file match
    if (m_fileNames.contains(normalizedPath, Qt::CaseInsensitive))
        return true;

    // Check for directory existence by seeing if any file starts with that path
    QString dirPath = normalizedPath + QLatin1Char('/');
    for (const QString& f : m_fileNames) {
        if (f.startsWith(dirPath, Qt::CaseInsensitive))
            return true;
    }

    return false;
}

ArchiveReader::File::File() : m_archive(ArchivePtr(archive_read_new(), archive_read_free)) {}
}  // namespace MMCZip
