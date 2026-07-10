// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 PineconeMC Contributors
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

#include "ResumingFileSink.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include "DownloadCache.h"
#include "FileSystem.h"
#include "net/Logging.h"

namespace Net {

ResumingFileSink::ResumingFileSink(QString finalPath, QUrl url, ::DownloadCache* cache)
    : m_finalPath(std::move(finalPath)), m_url(std::move(url)), m_cache(cache)
{
}

Task::State ResumingFileSink::init(QNetworkRequest& request)
{
    if (!FS::ensureFilePathExists(m_finalPath)) {
        qCCritical(taskNetLogC) << "Could not create folder for " + m_finalPath;
        m_fail_reason = "Could not create folder";
        return Task::State::Failed;
    }

    QString partPath;
    if (m_cache) {
        partPath = m_cache->getPartPath(m_url);
        m_existingSize = m_cache->getPartSize(m_url);
    } else {
        partPath = m_finalPath + ".part";
        QFileInfo info(partPath);
        m_existingSize = info.exists() ? info.size() : 0;
    }

    if (m_existingSize > 0) {
        request.setRawHeader("Range", QString("bytes=%1-").arg(m_existingSize).toLatin1());
        m_usedRange = true;
        qCDebug(taskNetLogC) << "Resuming download from byte" << m_existingSize << "for" << m_url.toString();
    }

    m_partFile = std::make_unique<QFile>(partPath);
    if (!m_partFile->open(QIODevice::Append)) {
        const auto error = QString("Could not open %1 for appending: %2").arg(partPath).arg(m_partFile->errorString());
        qCCritical(taskNetLogC) << error;
        m_fail_reason = error;
        return Task::State::Failed;
    }

    m_wroteAnyData = false;

    if (initAllValidators(request))
        return Task::State::Running;
    m_fail_reason = "Failed to initialize validators";
    return Task::State::Failed;
}

Task::State ResumingFileSink::write(QByteArray& data)
{
    if (!writeAllValidators(data)) {
        QString error = QString("Failed writing into %1: Validators failed").arg(m_partFile->fileName());
        qCCritical(taskNetLogC) << error;
        m_fail_reason = error;
        return Task::State::Failed;
    }

    if (m_partFile->write(data) != data.size()) {
        QString error = QString("Failed writing into %1: %2").arg(m_partFile->fileName()).arg(m_partFile->errorString());
        qCCritical(taskNetLogC) << error;
        m_fail_reason = error;
        return Task::State::Failed;
    }

    m_wroteAnyData = true;
    return Task::State::Running;
}

Task::State ResumingFileSink::abort()
{
    if (m_partFile) {
        m_partFile->close();
    }
    failAllValidators();
    // Keep the .part file for resume
    return Task::State::Failed;
}

Task::State ResumingFileSink::finalize(QNetworkReply& reply)
{
    bool gotFile = false;
    QVariant statusCodeV = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool validStatus = false;
    int statusCode = statusCodeV.toInt(&validStatus);

    if (validStatus) {
        gotFile = (statusCode == 200) || (statusCode == 206);
    }

    if (gotFile || m_wroteAnyData) {
        if (m_partFile) {
            m_partFile->close();
        }

        if (!finalizeAllValidators(reply)) {
            m_fail_reason = "Failed to finalize validators";
            return Task::State::Failed;
        }

        QString partPath = m_partFile ? m_partFile->fileName() : QString();

        // If we got 200 after Range, server doesn't support resume.
        // But validators passed, so the .part file is complete. Promote it.
        if (m_cache) {
            if (!m_cache->promotePart(m_url, m_finalPath)) {
                // Fallback: try direct rename
                QFileInfo finalInfo(m_finalPath);
                QDir().mkpath(finalInfo.absolutePath());
                if (QFileInfo::exists(m_finalPath))
                    QFile::remove(m_finalPath);
                if (!QFile::rename(partPath, m_finalPath)) {
                    m_fail_reason = QString("Failed to move %1 to %2").arg(partPath).arg(m_finalPath);
                    return Task::State::Failed;
                }
            }
        } else {
            QFileInfo finalInfo(m_finalPath);
            QDir().mkpath(finalInfo.absolutePath());
            if (QFileInfo::exists(m_finalPath))
                QFile::remove(m_finalPath);
            if (!QFile::rename(partPath, m_finalPath)) {
                m_fail_reason = QString("Failed to move %1 to %2").arg(partPath).arg(m_finalPath);
                return Task::State::Failed;
            }
        }

        m_partFile.reset();
        qCDebug(taskNetLogC) << "Download completed:" << m_url.toString() << "->" << m_finalPath;
    }

    m_partFile.reset();
    return Task::State::Succeeded;
}

bool ResumingFileSink::hasLocalData()
{
    QFileInfo info(m_finalPath);
    return info.exists() && info.size() != 0;
}

}  // namespace Net
