#pragma once
#include <QString>
#include <QSharedPointer>
#include <QMap>

struct MetaEntry
{
	QString base;
	QString path;
	QString md5sum;
	QString etag;
	quint64 last_changed_timestamp = 0;
};

typedef QSharedPointer<MetaEntry> MetaEntryPtr;

class HttpMetaCache
{
public:
	// supply path to the cache index file
	HttpMetaCache(QString path);
	~HttpMetaCache();
	MetaEntryPtr getEntryForResource(QString base, QString resource_path);
	void addEntry(QString base, QString resource_path, QString etag);
	void addBase(QString base, QString base_root);
private:
	void Save();
	void Load();
	struct EntryMap
	{
		QString base_path;
		QMap<QString, MetaEntryPtr> entry_list;
	};
	QMap<QString, EntryMap> m_entries;
	QString m_index_file;
};