// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Nic <73386479+nicyoong@users.noreply.github.com>
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

#pragma once

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>

/**
 * @brief A log file wrapper that stops growing once it reaches a maximum size.
 *
 * Writes up to maxSize bytes are passed through unchanged. When a write
 * would push the file past the limit, the file is filled up to the limit,
 * a one time notice is appended, and all further writes are discarded so
 * that a chatty logging loop cannot fill the user's disk (issue #752).
 */
class CappedLogFile {
   public:
    static constexpr qint64 DEFAULT_MAX_SIZE = 100 * 1024 * 1024;  // 100 MiB

    explicit CappedLogFile(const QString& fileName, qint64 maxSize = DEFAULT_MAX_SIZE) : m_file(fileName), m_maxSize(maxSize) {}
    ~CappedLogFile()
    {
        if (m_file.isOpen())
            m_file.close();
    }

    bool open(QIODevice::OpenMode mode) { return m_file.open(mode); }
    void write(const QByteArray& data)
    {
        if (m_limitReached)
            return;

        if (m_bytesWritten + data.size() > m_maxSize) {
            m_limitReached = true;

            qint64 remaining = m_maxSize - m_bytesWritten;
            if (remaining > 0) {
                m_file.write(data.constData(), remaining);
                m_bytesWritten += remaining;
            }

            QByteArray notice = QString(
                                    "\nThe log file has reached its maximum size of %1 bytes. "
                                    "Further log output is discarded for this session.\n")
                                    .arg(m_maxSize)
                                    .toUtf8();
            m_file.write(notice);
            m_file.flush();
            return;
        }

        m_file.write(data);
        m_bytesWritten += data.size();
    }
    void flush() { m_file.flush(); }
    void close() { m_file.close(); }
    QString errorString() const { return m_file.errorString(); }

    qint64 bytesWritten() const { return m_bytesWritten; }
    bool isLimitReached() const { return m_limitReached; }

   private:
    QFile m_file;
    qint64 m_maxSize;
    qint64 m_bytesWritten = 0;
    bool m_limitReached = false;
};
