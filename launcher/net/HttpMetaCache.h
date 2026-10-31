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

#pragma once

#include <QMap>
#include <QString>
#include <QTimer>
#include <memory>
#include <utility>

class HttpMetaCache;

class MetaEntry {
    friend class HttpMetaCache;

   protected:
    MetaEntry() = default;

   public:
    auto isStale() const -> bool { return m_stale; }
    void setStale(bool stale) { m_stale = stale; }

    auto getFullPath() const -> QString;

    auto getRemoteChangedTimestamp() const -> QString { return m_remoteChangedTimestamp; }
    void setRemoteChangedTimestamp(QString remoteChangedTimestamp) { m_remoteChangedTimestamp = std::move(remoteChangedTimestamp); }
    void setLocalChangedTimestamp(qint64 timestamp) { m_localChangedTimestamp = timestamp; }

    auto getETag() const -> QString { return m_etag; }
    void setETag(QString etag) { m_etag = std::move(etag); }

    auto getMD5Sum() const -> QString { return m_md5sum; }
    void setMD5Sum(QString md5sum) { m_md5sum = std::move(md5sum); }

    /* Whether the entry expires after some time (false) or not (true). */
    void makeEternal(bool eternal) { m_isEternal = eternal; }
    bool isEternal() const { return m_isEternal; }

    auto getCurrentAge() const -> qint64 { return m_currentAge; }
    void setCurrentAge(qint64 age) { m_currentAge = age; }

    auto getMaximumAge() const -> qint64 { return m_maxAge; }
    void setMaximumAge(qint64 age) { m_maxAge = age; }

    bool isExpired(qint64 offset) const { return !m_isEternal && (m_currentAge >= m_maxAge - offset); }

   protected:
    QString m_baseId;
    QString m_basePath;
    QString m_relativePath;
    QString m_md5sum;
    QString m_etag;

    qint64 m_localChangedTimestamp = 0;
    QString m_remoteChangedTimestamp;  // QString for now, RFC 2822 encoded time
    qint64 m_currentAge = 0;
    qint64 m_maxAge = 0;
    bool m_isEternal = false;

    bool m_stale = true;
};

using MetaEntryPtr = std::shared_ptr<MetaEntry>;

class HttpMetaCache : public QObject {
    Q_OBJECT
   public:
    // supply path to the cache index file
    explicit HttpMetaCache(QString path = {});
    ~HttpMetaCache() override;

    // get the entry solely from the cache
    // you probably don't want this, unless you have some specific caching needs.
    auto getEntry(const QString& base, const QString& resourcePath) -> MetaEntryPtr;

    // get the entry from cache and verify that it isn't stale (within reason)
    auto resolveEntry(const QString& base, QString resourcePath, bool checkMd5 = false) -> MetaEntryPtr;

    // returns `entry` if its file is unchanged or the md5sum still matches,
    // otherwise disowns it and returns a fresh stale entry for the same path.
    // may hash the file, so call it from a task
    auto verifyEntry(const MetaEntryPtr& entry) -> MetaEntryPtr;

    // add a previously resolved stale entry
    auto updateEntry(const MetaEntryPtr& staleEntry) -> bool;

    // evict selected entry from cache
    auto evictEntry(const MetaEntryPtr& entry) -> bool;
    bool evictAll();

    void addBase(const QString& base, const QString& baseRoot);

    // (re)start a timer that calls SaveNow later.
    void saveEventually();
    void load();

    auto getBasePath(const QString& base) -> QString;

   public slots:
    void saveNow();

   private:
    // create a new stale entry, given the parameters
    auto staleEntry(const QString& base, const QString& resourcePath) -> MetaEntryPtr;

    struct EntryMap {
        QString basePath;
        QMap<QString, MetaEntryPtr> entryList;
    };

    QMap<QString, EntryMap> m_entries;
    QString m_indexFile;
    QTimer m_saveBatchingTimer;
};
