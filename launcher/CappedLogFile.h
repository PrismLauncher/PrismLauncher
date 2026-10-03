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
 * @brief A log file wrapper that counts the bytes written to it.
 *
 * Intended as a drop in replacement for the launcher's QFile log file so
 * that a chatty logging loop cannot fill the user's disk (issue #752).
 */
class CappedLogFile {
   public:
    explicit CappedLogFile(const QString& fileName, qint64 maxSize) : m_file(fileName), m_maxSize(maxSize) {}
    ~CappedLogFile()
    {
        if (m_file.isOpen())
            m_file.close();
    }

    bool open(QIODevice::OpenMode mode) { return m_file.open(mode); }
    void write(const QByteArray& data)
    {
        m_file.write(data);
        m_bytesWritten += data.size();
    }
    void flush() { m_file.flush(); }
    void close() { m_file.close(); }
    QString errorString() const { return m_file.errorString(); }

    qint64 bytesWritten() const { return m_bytesWritten; }

   private:
    QFile m_file;
    qint64 m_maxSize;
    qint64 m_bytesWritten = 0;
};
