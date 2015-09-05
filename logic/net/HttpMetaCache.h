/* Copyright 2013-2015 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <QString>
#include <QMap>
#include <qtimer.h>
#include <memory>

#include "multimc_logic_export.h"

class HttpMetaCache;

struct MULTIMC_LOGIC_EXPORT MetaEntry
{
	QString base;
	QString path;
	QString md5sum;
	QString etag;
	qint64 local_changed_timestamp = 0;
	QString remote_changed_timestamp; // QString for now, RFC 2822 encoded time
	bool stale = true;
	QString getFullPath();
};

typedef std::shared_ptr<MetaEntry> MetaEntryPtr;

class MULTIMC_LOGIC_EXPORT HttpMetaCache : public QObject
{
	Q_OBJECT
public:
	// supply path to the cache index file
	HttpMetaCache(QString path);
	~HttpMetaCache();

	// get the entry solely from the cache
	// you probably don't want this, unless you have some specific caching needs.
	MetaEntryPtr getEntry(QString base, QString resource_path);

	// get the entry from cache and verify that it isn't stale (within reason)
	MetaEntryPtr resolveEntry(QString base, QString resource_path,
							  QString expected_etag = QString());

	// add a previously resolved stale entry
	bool updateEntry(MetaEntryPtr stale_entry);

	// evict selected entry from cache
	bool evictEntry(MetaEntryPtr entry);

	void addBase(QString base, QString base_root);

	// (re)start a timer that calls SaveNow later.
	void SaveEventually();
	void Load();
	QString getBasePath(QString base);
public
slots:
	void SaveNow();

private:
	// create a new stale entry, given the parameters
	MetaEntryPtr staleEntry(QString base, QString resource_path);
	struct EntryMap
	{
		QString base_path;
		QMap<QString, MetaEntryPtr> entry_list;
	};
	QMap<QString, EntryMap> m_entries;
	QString m_index_file;
	QTimer saveBatchingTimer;
};