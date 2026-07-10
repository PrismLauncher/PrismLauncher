// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Avenger Anubis (Ilya) <avenger.anubis@gmail.com>
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

#include "DownloadCache.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMutexLocker>

#include "net/Logging.h"

QJsonObject DownloadCacheEntry::toJson() const
{
    QJsonObject obj;
    obj["url"] = url;
    obj["cacheFileName"] = cacheFileName;
    obj["size"] = size;
    obj["lastAccessed"] = lastAccessed;
    obj["created"] = created;
    return obj;
}

DownloadCacheEntry DownloadCacheEntry::fromJson(const QJsonObject& obj)
{
    DownloadCacheEntry entry;
    entry.url = obj["url"].toString();
    entry.cacheFileName = obj["cacheFileName"].toString();
    entry.size = obj["size"].toInteger();
    entry.lastAccessed = obj["lastAccessed"].toInteger();
    entry.created = obj["created"].toInteger();
    return entry;
}

DownloadCache::DownloadCache(const QString& cacheDir)
    : m_cacheDir(cacheDir), m_partsDir(cacheDir + "/parts"), m_metadataPath(cacheDir + "/.metadata.json")
{
    QDir().mkpath(m_partsDir);
    QDir().mkpath(m_cacheDir);
    loadMetadata();
}

DownloadCache::~DownloadCache()
{
    saveMetadata();
}

QString DownloadCache::generateCacheKey(const QUrl& url) const
{
    QByteArray hash = QCryptographicHash::hash(url.toString().toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex().left(32));
}

QString DownloadCache::getPartPath(const QUrl& url) const
{
    return m_partsDir + "/" + generateCacheKey(url) + ".part";
}

qint64 DownloadCache::getPartSize(const QUrl& url) const
{
    QFileInfo info(getPartPath(url));
    return info.exists() ? info.size() : 0;
}

bool DownloadCache::hasPart(const QUrl& url) const
{
    return QFileInfo::exists(getPartPath(url));
}

QString DownloadCache::getCachePath(const QUrl& url) const
{
    QString key = generateCacheKey(url);
    return m_cacheDir + "/" + key;
}

bool DownloadCache::hasCache(const QUrl& url)
{
    QMutexLocker lock(&m_mutex);
    QString key = generateCacheKey(url);
    return m_entries.contains(key) && QFileInfo::exists(m_cacheDir + "/" + key);
}

bool DownloadCache::promotePart(const QUrl& url, const QString& finalPath)
{
    QString partPath = getPartPath(url);
    if (!QFileInfo::exists(partPath))
        return false;

    // Ensure destination directory exists
    QFileInfo finalInfo(finalPath);
    QDir().mkpath(finalInfo.absolutePath());

    // Remove destination if it exists
    if (QFileInfo::exists(finalPath)) {
        QFile::remove(finalPath);
    }

    // Rename .part to final destination (atomic on same filesystem)
    if (!QFile::rename(partPath, finalPath)) {
        qCCritical(taskNetLogC) << "Failed to rename" << partPath << "to" << finalPath;
        return false;
    }

    // Store in cache directory for future re-use
    QString key = generateCacheKey(url);
    QString cachePath = m_cacheDir + "/" + key;
    if (!QFile::copy(finalPath, cachePath))
        qCWarning(taskNetLogC) << "Failed to copy to cache:" << cachePath;

    DownloadCacheEntry entry;
    entry.url = url.toString();
    entry.cacheFileName = key;
    entry.size = QFileInfo(finalPath).size();
    entry.created = QDateTime::currentSecsSinceEpoch();
    entry.lastAccessed = entry.created;

    QMutexLocker lock(&m_mutex);
    m_entries[key] = entry;
    saveMetadata();

    qCDebug(taskNetLogC) << "Download cached:" << url.toString() << "->" << finalPath;
    return true;
}

bool DownloadCache::promoteCache(const QUrl& url, const QString& finalPath)
{
    QMutexLocker lock(&m_mutex);
    QString key = generateCacheKey(url);
    if (!m_entries.contains(key))
        return false;

    QString cachePath = m_cacheDir + "/" + key;
    if (!QFileInfo::exists(cachePath))
        return false;

    QFileInfo finalInfo(finalPath);
    QDir().mkpath(finalInfo.absolutePath());

    if (QFileInfo::exists(finalPath)) {
        QFile::remove(finalPath);
    }

    if (!QFile::rename(cachePath, finalPath)) {
        qCCritical(taskNetLogC) << "Failed to promote cache" << cachePath << "to" << finalPath;
        return false;
    }

    m_entries.remove(key);
    saveMetadata();

    qCDebug(taskNetLogC) << "Promoted cached download:" << url.toString() << "->" << finalPath;
    return true;
}

bool DownloadCache::serveFromCache(const QUrl& url, const QString& finalPath)
{
    QMutexLocker lock(&m_mutex);
    QString key = generateCacheKey(url);
    if (!m_entries.contains(key))
        return false;

    QString cachePath = m_cacheDir + "/" + key;
    if (!QFileInfo::exists(cachePath))
        return false;

    QFileInfo finalInfo(finalPath);
    QDir().mkpath(finalInfo.absolutePath());

    if (QFileInfo::exists(finalPath))
        QFile::remove(finalPath);

    if (!QFile::copy(cachePath, finalPath)) {
        qCCritical(taskNetLogC) << "Failed to serve from cache:" << cachePath << "->" << finalPath;
        return false;
    }

    m_entries[key].lastAccessed = QDateTime::currentSecsSinceEpoch();
    saveMetadata();

    qCDebug(taskNetLogC) << "Served from cache:" << url.toString();
    return true;
}

void DownloadCache::removePart(const QUrl& url)
{
    QString partPath = getPartPath(url);
    if (QFileInfo::exists(partPath)) {
        QFile::remove(partPath);
    }
}

void DownloadCache::removeCache(const QUrl& url)
{
    QMutexLocker lock(&m_mutex);
    QString key = generateCacheKey(url);
    if (m_entries.contains(key)) {
        QString cachePath = m_cacheDir + "/" + key;
        if (QFileInfo::exists(cachePath)) {
            QFile::remove(cachePath);
        }
        m_entries.remove(key);
        saveMetadata();
    }
}

void DownloadCache::cleanup(qint64 maxSizeBytes, int maxAgeHours)
{
    QMutexLocker lock(&m_mutex);

    qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 maxAgeSecs = static_cast<qint64>(maxAgeHours) * 3600;

    // Remove expired entries first
    QMutableHashIterator<QString, DownloadCacheEntry> it(m_entries);
    while (it.hasNext()) {
        it.next();
        if (maxAgeHours > 0 && (now - it.value().lastAccessed) > maxAgeSecs) {
            QString cachePath = m_cacheDir + "/" + it.key();
            if (QFileInfo::exists(cachePath)) {
                QFile::remove(cachePath);
            }
            qCDebug(taskNetLogC) << "Removed expired cache entry:" << it.value().url;
            it.remove();
        }
    }

    // Also clean up orphaned .part files (older than max age)
    if (maxAgeHours > 0) {
        QDirIterator partIt(m_partsDir, { "*.part" }, QDir::Files);
        while (partIt.hasNext()) {
            partIt.next();
            QFileInfo fi = partIt.fileInfo();
            if (fi.lastModified().toSecsSinceEpoch() < (now - maxAgeSecs)) {
                QFile::remove(fi.absoluteFilePath());
                qCDebug(taskNetLogC) << "Removed expired .part file:" << fi.fileName();
            }
        }
    }

    saveMetadata();

    // If still over size limit, evict oldest
    if (maxSizeBytes > 0 && totalCacheSize() > maxSizeBytes) {
        QList<DownloadCacheEntry> sorted = m_entries.values();
        std::sort(sorted.begin(), sorted.end(),
                  [](const DownloadCacheEntry& a, const DownloadCacheEntry& b) { return a.lastAccessed < b.lastAccessed; });

        while (totalCacheSize() > maxSizeBytes && !sorted.isEmpty()) {
            auto oldest = sorted.takeFirst();
            QString cachePath = m_cacheDir + "/" + oldest.cacheFileName;
            if (QFileInfo::exists(cachePath)) {
                QFile::remove(cachePath);
            }
            m_entries.remove(oldest.cacheFileName);
            qCDebug(taskNetLogC) << "Evicted oldest cache entry:" << oldest.url;
        }
        saveMetadata();
    }
}

qint64 DownloadCache::totalCacheSize() const
{
    qint64 total = 0;
    for (auto& entry : m_entries) {
        total += entry.size;
    }
    return total;
}

void DownloadCache::loadMetadata()
{
    QFile file(m_metadataPath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject())
        return;

    QJsonArray arr = doc.object()["entries"].toArray();
    for (const auto& val : arr) {
        DownloadCacheEntry entry = DownloadCacheEntry::fromJson(val.toObject());
        m_entries[entry.cacheFileName] = entry;
    }
}

void DownloadCache::saveMetadata()
{
    QJsonArray arr;
    for (auto& entry : m_entries) {
        arr.append(entry.toJson());
    }

    QJsonObject root;
    root["entries"] = arr;
    root["version"] = 1;

    QJsonDocument doc(root);
    QFile file(m_metadataPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void DownloadCache::touchAccessTime(const QString& cacheKey)
{
    if (m_entries.contains(cacheKey)) {
        m_entries[cacheKey].lastAccessed = QDateTime::currentSecsSinceEpoch();
    }
}
