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

class HttpMetaCache;

class MetaEntry {
    friend class HttpMetaCache;

   protected:
    MetaEntry() = default;

   public:
    using Ptr = std::shared_ptr<MetaEntry>;

    bool isStale() const { return m_stale; }
    void setStale(bool stale) { m_stale = stale; }

    QString getFullPath() const;

    QString getRemoteChangedTimestamp() const { return m_remote_changed_timestamp; }
    void setRemoteChangedTimestamp(QString remote_changed_timestamp) { m_remote_changed_timestamp = remote_changed_timestamp; }
    void setLocalChangedTimestamp(qint64 timestamp) { m_local_changed_timestamp = timestamp; }

    QString getETag() const { return m_etag; }
    void setETag(QString etag) { m_etag = etag; }

    QString getMD5Sum() const { return m_md5sum; }
    void setMD5Sum(QString md5sum) { m_md5sum = md5sum; }

    /* Whether the entry expires after some time (false) or not (true). */
    void makeEternal(bool eternal) { m_is_eternal = eternal; }
    [[nodiscard]] bool isEternal() const { return m_is_eternal; }

    qint64 getCurrentAge() const { return m_current_age; }
    void setCurrentAge(qint64 age) { m_current_age = age; }

    qint64 getMaximumAge() const { return m_max_age; }
    void setMaximumAge(qint64 age) { m_max_age = age; }

    bool isExpired(qint64 offset) const { return !m_is_eternal && (m_current_age >= m_max_age - offset); }

   protected:
    QString m_baseId;
    QString m_basePath;
    QString m_relativePath;
    QString m_md5sum;
    QString m_etag;

    qint64 m_local_changed_timestamp = 0;
    QString m_remote_changed_timestamp;  // QString for now, RFC 2822 encoded time
    qint64 m_current_age = 0;
    qint64 m_max_age = 0;
    bool m_is_eternal = false;

    bool m_stale = true;
};

class HttpMetaCache : public QObject {
    Q_OBJECT
   public:
    // supply path to the cache index file
    HttpMetaCache(QString path = QString());
    ~HttpMetaCache() override;

    // get the entry solely from the cache
    // you probably don't want this, unless you have some specific caching needs.
    MetaEntry::Ptr getEntry(QString base, QString resource_path);

    // get the entry from cache and verify that it isn't stale (within reason)
    MetaEntry::Ptr resolveEntry(QString base, QString resource_path, QString expected_etag = QString());

    // add a previously resolved stale entry
    bool updateEntry(MetaEntry::Ptr stale_entry);

    // evict selected entry from cache
    bool evictEntry(MetaEntry::Ptr entry);
    void evictAll();

    void addBase(QString base, QString base_root);

    // (re)start a timer that calls SaveNow later.
    void SaveEventually();
    void Load();

    QString getBasePath(QString base);

   public slots:
    void SaveNow();

   private:
    // create a new stale entry, given the parameters
    MetaEntry::Ptr staleEntry(QString base, QString resource_path);

    struct EntryMap {
        QString base_path;
        QMap<QString, MetaEntry::Ptr> entry_list;
    };

    QMap<QString, EntryMap> m_entries;
    QString m_index_file;
    QTimer saveBatchingTimer;
};
