// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include "FileSink.h"

#include "FileSystem.h"

#include "net/Logging.h"

#include <QDateTime>
#include <QFileInfo>

namespace Net {

Task::State FileSink::init(QNetworkRequest& request)
{
    auto result = initCache(request);
    if (result != Task::State::Running) {
        return result;
    }

    m_part_path = m_filename + ".part";
    m_wroteAnyData = false;
    m_part_size = -1;

    if (m_output_file && m_output_file->isOpen()) {
        m_output_file->close();
    }
    m_output_file.reset();

    // create destination path and initialize validators
    if (!FS::ensureFilePathExists(m_part_path)) {
        qCCritical(taskNetLogC) << "Could not create folder for " + m_part_path;
        m_fail_reason = "Could not create folder";
        return Task::State::Failed;
    }
    if (!initAllValidators(request)) {
        m_fail_reason = "Failed to initialize validators";
        return Task::State::Failed;
    }

    QFileInfo partInfo(m_part_path);
    if (partInfo.exists()) {
        auto modified = partInfo.lastModified();
        if (modified.isValid() && modified.daysTo(QDateTime::currentDateTimeUtc()) > 7) {
            qCWarning(taskNetLogC) << "Removing stale partial download" << m_part_path;
            QFile::remove(m_part_path);
            partInfo.refresh();
        }
    }

    // if we already have partial data, pre-seed validators and continue from there
    if (partInfo.exists() && partInfo.size() > 0) {
        QFile seedFile(m_part_path);
        if (!seedFile.open(QIODevice::ReadOnly)) {
            qCWarning(taskNetLogC) << "Failed to open partial file for resume, restarting:" << m_part_path;
            if (!QFile::remove(m_part_path) && QFile::exists(m_part_path)) {
                qCCritical(taskNetLogC) << "Failed to remove invalid partial file" << m_part_path;
                m_fail_reason = "Failed to reset partial file";
                return Task::State::Failed;
            }
            if (!resetAllValidators()) {
                m_fail_reason = "Failed to reset validators";
                return Task::State::Failed;
            }
        } else {
            constexpr qint64 CHUNK_BUFFER_SIZE = 1024 * 1024;
            bool readError = false;
            while (!seedFile.atEnd()) {
                auto chunk = seedFile.read(CHUNK_BUFFER_SIZE);
                if (seedFile.error() != QFile::NoError) {
                    readError = true;
                    break;
                }
                if (chunk.isEmpty()) {
                    if (!seedFile.atEnd()) {
                        readError = true;
                    }
                    break;
                }
                if (!writeAllValidators(chunk)) {
                    readError = true;
                    break;
                }
            }

            if (readError) {
                qCWarning(taskNetLogC) << "Partial download is invalid, restarting:" << m_part_path;
                seedFile.close();
                if (!QFile::remove(m_part_path) && QFile::exists(m_part_path)) {
                    qCCritical(taskNetLogC) << "Failed to remove invalid partial file" << m_part_path;
                    m_fail_reason = "Failed to reset partial file";
                    return Task::State::Failed;
                }
                if (!resetAllValidators()) {
                    m_fail_reason = "Failed to reset validators";
                    return Task::State::Failed;
                }
            } else {
                m_part_size = seedFile.size();
                qCDebug(taskNetLogC) << "Resuming from partial file" << m_part_path << "with size" << m_part_size;
                seedFile.close();
            }
        }
    }

    m_output_file = std::make_unique<QFile>(m_part_path);
    QIODevice::OpenMode mode = QIODevice::WriteOnly;
    if (QFileInfo(m_part_path).exists()) {
        mode |= QIODevice::Append;
    } else {
        mode |= QIODevice::Truncate;
    }
    if (!m_output_file->open(mode)) {
        qCCritical(taskNetLogC) << "Could not open" << m_part_path << "for writing";
        m_fail_reason = "Could not open file";
        return Task::State::Failed;
    }

    if (m_part_size < 0) {
        m_part_size = m_output_file->size();
    }
    return Task::State::Running;
}

Task::State FileSink::write(QByteArray& data)
{
    if (!m_output_file || !m_output_file->isOpen() || !writeAllValidators(data) || m_output_file->write(data) != data.size()) {
        qCCritical(taskNetLogC) << "Failed writing into" << m_part_path;
        if (m_output_file && m_output_file->isOpen()) {
            m_output_file->close();
        }
        m_output_file.reset();
        m_wroteAnyData = false;
        m_fail_reason = "Failed to write output";
        return Task::State::Failed;
    }

    m_wroteAnyData = true;
    m_part_size += data.size();
    return Task::State::Running;
}

Task::State FileSink::abort()
{
    if (m_output_file && m_output_file->isOpen()) {
        m_output_file->close();
    }
    failAllValidators();
    m_part_size = currentLocalSize();
    m_wroteAnyData = false;
    return Task::State::Failed;
}

Task::State FileSink::finalize(QNetworkReply& reply)
{
    bool gotFile = false;
    QVariant statusCodeV = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool validStatus = false;
    int statusCode = statusCodeV.toInt(&validStatus);
    if (validStatus) {
        // this leaves out 304 Not Modified
        gotFile = statusCode == 200 || statusCode == 203 || statusCode == 206;
    }

    // if we wrote any data to the temporary file, try to commit it to the destination.
    // if it actually got a proper file, we write it even if it was empty
    if (gotFile || m_wroteAnyData) {
        if (m_output_file && m_output_file->isOpen()) {
            if (!m_output_file->flush()) {
                qCCritical(taskNetLogC) << "Failed flushing partial file" << m_part_path;
                m_fail_reason = "Failed to flush output file";
                return Task::State::Failed;
            }
            m_output_file->close();
        }

        if (!finalizeAllValidators(reply)) {
            QFile::remove(m_part_path);
            m_fail_reason = "Failed to finalize validators";
            return Task::State::Failed;
        }

        QString backup_path;
        bool destination_backed_up = false;
        if (QFile::exists(m_filename)) {
            backup_path = m_filename + ".old";

            if (QFile::exists(backup_path) && !QFile::remove(backup_path)) {
                qCCritical(taskNetLogC) << "Failed to prepare backup path" << backup_path;
                m_fail_reason = "Failed to prepare file replacement backup";
                return Task::State::Failed;
            }

            if (!QFile::rename(m_filename, backup_path)) {
                qCCritical(taskNetLogC) << "Failed to backup destination file" << m_filename;
                m_fail_reason = "Failed to replace destination file";
                return Task::State::Failed;
            }
            destination_backed_up = true;
        }

        if (!QFile::rename(m_part_path, m_filename)) {
            qCCritical(taskNetLogC) << "Failed to commit changes to" << m_filename;
            if (destination_backed_up && !QFile::rename(backup_path, m_filename)) {
                qCCritical(taskNetLogC) << "Failed to restore destination file from backup" << backup_path;
            }
            m_fail_reason = "Failed to commit changes";
            return Task::State::Failed;
        }

        if (destination_backed_up && !QFile::remove(backup_path)) {
            qCWarning(taskNetLogC) << "Failed to remove destination backup file" << backup_path;
        }
    }

    m_output_file.reset();
    m_part_size = -1;

    return finalizeCache(reply);
}

Task::State FileSink::initCache(QNetworkRequest&)
{
    return Task::State::Running;
}

Task::State FileSink::finalizeCache(QNetworkReply&)
{
    return Task::State::Succeeded;
}

bool FileSink::hasLocalData()
{
    QFileInfo info(m_filename);
    return info.exists() && info.size() != 0;
}

qint64 FileSink::currentLocalSize()
{
    QFileInfo partInfo(m_part_path);
    if (!partInfo.exists()) {
        m_part_size = 0;
        return 0;
    }

    auto actualSize = partInfo.size();
    if (m_part_size < 0 || m_part_size != actualSize) {
        m_part_size = actualSize;
    }
    return m_part_size;
}

void FileSink::truncate()
{
    if (m_output_file && m_output_file->isOpen()) {
        m_output_file->close();
    }

    QFile file(m_part_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCCritical(taskNetLogC) << "Failed truncating partial file" << m_part_path;
        m_fail_reason = "Failed to truncate partial file";
        m_output_file.reset();
        return;
    }

    file.close();
    m_part_size = 0;
    m_wroteAnyData = false;

    if (!resetAllValidators()) {
        m_fail_reason = "Failed to reset validators";
        m_output_file.reset();
        return;
    }

    m_output_file = std::make_unique<QFile>(m_part_path);
    if (!m_output_file->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qCCritical(taskNetLogC) << "Failed to reopen partial file" << m_part_path;
        m_fail_reason = "Failed to reopen partial file";
        m_output_file.reset();
    }
}
}  // namespace Net
