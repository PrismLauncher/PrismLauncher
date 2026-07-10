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

#pragma once

#include <QByteArray>
#include <QDir>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QMutex>
#include <QString>
#include <QUrl>

class DownloadCacheEntry {
   public:
    QString url;
    QString cacheFileName;
    qint64 size = 0;
    qint64 lastAccessed = 0;
    qint64 created = 0;

    QJsonObject toJson() const;
    static DownloadCacheEntry fromJson(const QJsonObject& obj);
};

class DownloadCache {
   public:
    explicit DownloadCache(const QString& cacheDir);
    ~DownloadCache();

    QString getPartPath(const QUrl& url) const;
    qint64 getPartSize(const QUrl& url) const;
    bool hasPart(const QUrl& url) const;
    QString getCachePath(const QUrl& url) const;
    bool hasCache(const QUrl& url);

    bool promotePart(const QUrl& url, const QString& finalPath);
    bool promoteCache(const QUrl& url, const QString& finalPath);
    void removePart(const QUrl& url);
    void removeCache(const QUrl& url);

    void cleanup(qint64 maxSizeBytes, int maxAgeHours);
    qint64 totalCacheSize() const;

    QString cacheDir() const { return m_cacheDir; }
    QString partsDir() const { return m_partsDir; }

   private:
    QString generateCacheKey(const QUrl& url) const;
    void loadMetadata();
    void saveMetadata();
    void touchAccessTime(const QString& cacheKey);

    QString m_cacheDir;
    QString m_partsDir;
    QString m_metadataPath;

    QHash<QString, DownloadCacheEntry> m_entries;
    mutable QMutex m_mutex;
};
