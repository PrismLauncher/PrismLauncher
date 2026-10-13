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

#include "HttpMetaCache.h"
#include "FileSystem.h"
#include "Json.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

#include <QDebug>
#include <utility>

#include "modplatform/helpers/HashUtils.h"
#include "net/Logging.h"

auto MetaEntry::getFullPath() const -> QString
{
    // FIXME: make local?
    return FS::PathCombine(m_basePath, m_relativePath);
}

HttpMetaCache::HttpMetaCache(QString path) : m_indexFile(std::move(path))
{
    m_saveBatchingTimer.setSingleShot(true);
    m_saveBatchingTimer.setTimerType(Qt::VeryCoarseTimer);

    connect(&m_saveBatchingTimer, &QTimer::timeout, this, &HttpMetaCache::saveNow);
}

HttpMetaCache::~HttpMetaCache()
{
    m_saveBatchingTimer.stop();
    saveNow();
}

auto HttpMetaCache::getEntry(const QString& base, const QString& resourcePath) -> MetaEntryPtr
{
    // no base. no base path. can't store
    if (!m_entries.contains(base)) {
        // TODO: log problem
        return {};
    }

    EntryMap& map = m_entries[base];
    if (map.entryList.contains(resourcePath)) {
        return map.entryList[resourcePath];
    }

    return {};
}

auto HttpMetaCache::resolveEntry(const QString& base, QString resourcePath, bool checkMd5) -> MetaEntryPtr
{
    resourcePath = FS::RemoveInvalidPathChars(resourcePath);
    auto entry = getEntry(base, resourcePath);
    // it's not present? generate a default stale entry
    if (!entry) {
        return staleEntry(base, resourcePath);
    }

    auto& selectedBase = m_entries[base];
    auto realPath = FS::PathCombine(selectedBase.basePath, resourcePath);
    QFileInfo finfo(realPath);

    // is the file really there? if not -> stale
    if (!finfo.isFile() || !finfo.isReadable()) {
        // if the file doesn't exist, we disown the entry
        selectedBase.entryList.remove(resourcePath);
        return staleEntry(base, resourcePath);
    }

    // if the file changed, check md5sum
    qint64 fileLastChanged = finfo.lastModified().toUTC().toMSecsSinceEpoch();
    if (fileLastChanged != entry->m_localChangedTimestamp) {
        if (entry->m_md5sum.isEmpty()) {
            selectedBase.entryList.remove(resourcePath);
            return staleEntry(base, resourcePath);
        }
        if (checkMd5) {
            auto verified = verifyEntry(entry);
            if (verified != entry) {
                return verified;
            }
        }
    }

    // Get rid of old entries, to prevent cache problems
    auto currentTime = QDateTime::currentSecsSinceEpoch();
    if (entry->isExpired(currentTime - (fileLastChanged / 1000))) {
        qCWarning(taskNetLogC) << "[HttpMetaCache]"
                               << "Removing cache entry because of old age!";
        selectedBase.entryList.remove(resourcePath);
        return staleEntry(base, resourcePath);
    }

    // entry passed all the checks we cared about.
    entry->m_basePath = getBasePath(base);
    return entry;
}

auto HttpMetaCache::verifyEntry(const MetaEntryPtr& entry) -> MetaEntryPtr
{
    // no md5sum to check against, so we can't verify it
    if (entry->m_md5sum.isEmpty()) {
        return entry;
    }
    auto& selectedBase = m_entries[entry->m_baseId];
    auto realPath = FS::PathCombine(selectedBase.basePath, entry->m_relativePath);

    QFileInfo finfo(realPath);
    qint64 fileLastChanged = finfo.lastModified().toUTC().toMSecsSinceEpoch();
    if (fileLastChanged == entry->m_localChangedTimestamp) {
        return entry;
    }

    // the file changed: disown the entry and hand out a fresh stale one,
    // so other holders of the old entry are left untouched
    QFile input(realPath);
    if (!input.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file" << input.fileName() << "for reading:" << input.errorString();
        selectedBase.entryList.remove(entry->m_relativePath);
        return staleEntry(entry->m_baseId, entry->m_relativePath);
    }
    if (entry->m_md5sum != Hashing::hash(&input, Hashing::Algorithm::Md5)) {
        selectedBase.entryList.remove(entry->m_relativePath);
        return staleEntry(entry->m_baseId, entry->m_relativePath);
    }

    // md5sums matched... keep entry and save the new state to file
    entry->m_localChangedTimestamp = fileLastChanged;
    saveEventually();
    return entry;
}

auto HttpMetaCache::updateEntry(const MetaEntryPtr& staleEntry) -> bool
{
    if (!m_entries.contains(staleEntry->m_baseId)) {
        qCCritical(taskHttpMetaCacheLogC) << "Cannot add entry with unknown base:" << staleEntry->m_baseId.toLocal8Bit();
        return false;
    }

    if (staleEntry->m_stale) {
        qCCritical(taskHttpMetaCacheLogC) << "Cannot add stale entry:" << staleEntry->getFullPath().toLocal8Bit();
        return false;
    }

    m_entries[staleEntry->m_baseId].entryList[staleEntry->m_relativePath] = staleEntry;
    saveEventually();

    return true;
}

auto HttpMetaCache::evictEntry(const MetaEntryPtr& entry) -> bool
{
    if (!entry) {
        return false;
    }

    entry->m_stale = true;
    saveEventually();
    return true;
}

// returns true on success, false otherwise
auto HttpMetaCache::evictAll() -> bool
{
    bool ret = true;
    for (QString& base : m_entries.keys()) {
        EntryMap& map = m_entries[base];
        qCDebug(taskHttpMetaCacheLogC) << "Evicting base" << base;
        for (const auto& entry : map.entryList) {
            if (!evictEntry(entry)) {
                qCWarning(taskHttpMetaCacheLogC) << "Unexpected missing cache entry" << entry->m_basePath;
            }
        }
        map.entryList.clear();
        // AND all return codes together so the result is true iff all runs of deletePath() are true
        ret &= FS::deleteContents(map.basePath);
    }
    return ret;
}

auto HttpMetaCache::staleEntry(const QString& base, const QString& resourcePath) -> MetaEntryPtr
{
    auto* foo = new MetaEntry();
    foo->m_baseId = base;
    foo->m_basePath = getBasePath(base);
    foo->m_relativePath = resourcePath;
    foo->m_stale = true;

    return MetaEntryPtr(foo);
}

void HttpMetaCache::addBase(const QString& base, const QString& baseRoot)
{
    // TODO: report error
    if (m_entries.contains(base)) {
        return;
    }

    // TODO: check if the base path is valid
    EntryMap foo;
    foo.basePath = baseRoot;
    m_entries[base] = foo;
}

auto HttpMetaCache::getBasePath(const QString& base) -> QString
{
    if (m_entries.contains(base)) {
        return m_entries[base].basePath;
    }

    return {};
}

void HttpMetaCache::load()
{
    if (m_indexFile.isNull()) {
        return;
    }

    QFile index(m_indexFile);
    if (!index.open(QIODevice::ReadOnly)) {
        return;
    }

    auto json = Json::requireObject(index.readAll(), "HttpMetaCache");

    // Fail if the JSON is invalid or the root is not an object.
    if (!json) {
        qCritical() << json.error();
        return;
    }

    auto root = json.value();

    // check file version first
    auto versionVal = root["version"].toString();
    if (versionVal != "1") {
        return;
    }

    // read the entry array
    auto array = root["entries"].toArray();
    for (auto element : array) {
        auto elementObj = element.toObject();
        auto base = elementObj["base"].toString();
        if (!m_entries.contains(base)) {
            continue;
        }

        auto& entrymap = m_entries[base];

        auto* foo = new MetaEntry();
        foo->m_baseId = base;
        foo->m_relativePath = elementObj["path"].toString();
        foo->m_md5sum = elementObj["md5sum"].toString();
        foo->m_etag = elementObj["etag"].toString();
        foo->m_localChangedTimestamp = elementObj["last_changed_timestamp"].toDouble();
        foo->m_remoteChangedTimestamp = elementObj["remote_changed_timestamp"].toString();

        foo->makeEternal(elementObj[QStringLiteral("eternal")].toBool());
        if (!foo->isEternal()) {
            foo->m_currentAge = elementObj["current_age"].toDouble();
            foo->m_maxAge = elementObj["max_age"].toDouble();
        }

        // presumed innocent until closer examination
        foo->m_stale = false;

        entrymap.entryList[foo->m_relativePath] = MetaEntryPtr(foo);
    }
}

void HttpMetaCache::saveEventually()
{
    // reset the save timer
    m_saveBatchingTimer.stop();
    m_saveBatchingTimer.start(30000);
}

void HttpMetaCache::saveNow()
{
    if (m_indexFile.isNull()) {
        return;
    }

    qCDebug(taskHttpMetaCacheLogC) << "Saving metacache with" << m_entries.size() << "entries";

    QJsonObject toplevel;
    Json::writeString(toplevel, "version", "1");

    QJsonArray entriesArr;
    for (const auto& group : m_entries) {
        for (const auto& entry : group.entryList) {
            // do not save stale entries. they are dead.
            if (entry->m_stale) {
                continue;
            }

            QJsonObject entryObj;
            Json::writeString(entryObj, "base", entry->m_baseId);
            Json::writeString(entryObj, "path", entry->m_relativePath);
            Json::writeString(entryObj, "md5sum", entry->m_md5sum);
            Json::writeString(entryObj, "etag", entry->m_etag);
            entryObj.insert("last_changed_timestamp", QJsonValue(static_cast<double>(entry->m_localChangedTimestamp)));
            if (!entry->m_remoteChangedTimestamp.isEmpty()) {
                entryObj.insert("remote_changed_timestamp", QJsonValue(entry->m_remoteChangedTimestamp));
            }
            if (entry->isEternal()) {
                entryObj.insert("eternal", true);
            } else {
                entryObj.insert("current_age", QJsonValue(static_cast<double>(entry->m_currentAge)));
                entryObj.insert("max_age", QJsonValue(static_cast<double>(entry->m_maxAge)));
            }
            entriesArr.append(entryObj);
        }
    }
    toplevel.insert("entries", entriesArr);

    auto res = Json::write(toplevel, m_indexFile);
    if (!res) {
        qCWarning(taskHttpMetaCacheLogC) << "Error writing cache:" << res.error();
    }
}
