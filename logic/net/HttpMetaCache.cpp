#include "MultiMC.h"
#include "HttpMetaCache.h"
#include <pathutils.h>

#include <QFileInfo>
#include <QFile>
#include <QTemporaryFile>
#include <QSaveFile>
#include <QDateTime>
#include <QCryptographicHash>

#include <logger/QsLog.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

QString MetaEntry::getFullPath()
{
	return PathCombine(MMC->metacache()->getBasePath(base), path);
}


HttpMetaCache::HttpMetaCache(QString path)
	:QObject()
{
	m_index_file = path;
	saveBatchingTimer.setSingleShot(true);
	saveBatchingTimer.setTimerType(Qt::VeryCoarseTimer);
	connect(&saveBatchingTimer,SIGNAL(timeout()),SLOT(SaveNow()));
}

HttpMetaCache::~HttpMetaCache()
{
	saveBatchingTimer.stop();
	SaveNow();
}

MetaEntryPtr HttpMetaCache::getEntry ( QString base, QString resource_path )
{
	// no base. no base path. can't store
	if(!m_entries.contains(base))
	{
		// TODO: log problem
		return MetaEntryPtr();
	}
	EntryMap & map = m_entries[base];
	if(map.entry_list.contains(resource_path))
	{
		return map.entry_list[resource_path];
	}
	return MetaEntryPtr();
}

MetaEntryPtr HttpMetaCache::resolveEntry ( QString base, QString resource_path, QString expected_etag )
{
	auto entry = getEntry(base, resource_path);
	// it's not present? generate a default stale entry
	if(!entry)
	{
		return staleEntry(base, resource_path);
	}
	
	auto & selected_base = m_entries[base];
	QString real_path = PathCombine(selected_base.base_path, resource_path);
	QFileInfo finfo(real_path);
	
	// is the file really there? if not -> stale
	if(!finfo.isFile() || !finfo.isReadable())
	{
		// if the file doesn't exist, we disown the entry
		selected_base.entry_list.remove(resource_path);
		return staleEntry(base, resource_path);
	}
	
	if(!expected_etag.isEmpty() && expected_etag != entry->etag)
	{
		// if the etag doesn't match expected, we disown the entry
		selected_base.entry_list.remove(resource_path);
		return staleEntry(base, resource_path);
	}
	
	// if the file changed, check md5sum
	qint64 file_last_changed = finfo.lastModified().toUTC().toMSecsSinceEpoch();
	if(file_last_changed != entry->local_changed_timestamp)
	{
		QFile input(real_path);
		input.open(QIODevice::ReadOnly);
		QString md5sum = QCryptographicHash::hash(input.readAll(), QCryptographicHash::Md5).toHex().constData();
		if(entry->md5sum != md5sum)
		{
			selected_base.entry_list.remove(resource_path);
			return staleEntry(base, resource_path);
		}
		// md5sums matched... keep entry and save the new state to file
		entry->local_changed_timestamp = file_last_changed;
		SaveEventually();
	}

	// entry passed all the checks we cared about.
	return entry;
}

bool HttpMetaCache::updateEntry ( MetaEntryPtr stale_entry )
{
	if(!m_entries.contains(stale_entry->base))
	{
		QLOG_ERROR() << "Cannot add entry with unknown base: " << stale_entry->base.toLocal8Bit();
		return false;
	}
	if(stale_entry->stale)
	{
		QLOG_ERROR() << "Cannot add stale entry: " << stale_entry->getFullPath().toLocal8Bit();
		return false;
	}
	m_entries[stale_entry->base].entry_list[stale_entry->path] = stale_entry;
	SaveEventually();
	return true;
}

MetaEntryPtr HttpMetaCache::staleEntry(QString base, QString resource_path)
{
	auto foo = new MetaEntry;
	foo->base = base;
	foo->path = resource_path;
	foo->stale = true;
	return MetaEntryPtr(foo);
}

void HttpMetaCache::addBase ( QString base, QString base_root )
{
	// TODO: report error
	if(m_entries.contains(base))
		return;
	// TODO: check if the base path is valid
	EntryMap foo;
	foo.base_path = base_root;
	m_entries[base] = foo;
}

QString HttpMetaCache::getBasePath ( QString base )
{
	if(m_entries.contains(base))
	{
		return m_entries[base].base_path;
	}
	return QString();
}


void HttpMetaCache::Load()
{
	QFile index(m_index_file);
	if(!index.open(QIODevice::ReadOnly))
		return;
	
	QJsonDocument json = QJsonDocument::fromJson(index.readAll());
	if(!json.isObject())
		return;
	auto root = json.object();
	// check file version first
	auto version_val =root.value("version");
	if(!version_val.isString())
		return;
	if(version_val.toString() != "1")
		return;
	
	// read the entry array
	auto entries_val =root.value("entries");
	if(!entries_val.isArray())
		return;
	QJsonArray array = entries_val.toArray();
	for(auto element: array)
	{
		if(!element.isObject())
			return;
		auto element_obj = element.toObject();
		QString base = element_obj.value("base").toString();
		if(!m_entries.contains(base))
			continue;
		auto & entrymap = m_entries[base];
		auto foo = new MetaEntry;
		foo->base = base;
		QString path = foo->path = element_obj.value("path").toString();
		foo->md5sum = element_obj.value("md5sum").toString();
		foo->etag = element_obj.value("etag").toString();
		foo->local_changed_timestamp = element_obj.value("last_changed_timestamp").toDouble();
		foo->remote_changed_timestamp = element_obj.value("remote_changed_timestamp").toString();
		// presumed innocent until closer examination
		foo->stale = false;
		entrymap.entry_list[path] = MetaEntryPtr( foo );
	}
}

void HttpMetaCache::SaveEventually()
{
	// reset the save timer
	saveBatchingTimer.stop();
	saveBatchingTimer.start(30000);
}

void HttpMetaCache::SaveNow()
{
	QSaveFile tfile(m_index_file);
	if(!tfile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return;
	QJsonObject toplevel;
	toplevel.insert("version",QJsonValue(QString("1")));
	QJsonArray entriesArr;
	for(auto group : m_entries)
	{
		for(auto entry : group.entry_list)
		{
			QJsonObject entryObj;
			entryObj.insert("base", QJsonValue(entry->base));
			entryObj.insert("path", QJsonValue(entry->path));
			entryObj.insert("md5sum", QJsonValue(entry->md5sum));
			entryObj.insert("etag", QJsonValue(entry->etag));
			entryObj.insert("last_changed_timestamp", QJsonValue(double(entry->local_changed_timestamp)));
			if(!entry->remote_changed_timestamp.isEmpty())
				entryObj.insert("remote_changed_timestamp", QJsonValue(entry->remote_changed_timestamp));
			entriesArr.append(entryObj);
		}
	}
	toplevel.insert("entries",entriesArr);
	QJsonDocument doc(toplevel);
	QByteArray jsonData = doc.toJson();
	qint64 result = tfile.write(jsonData);
	if(result == -1)
		return;
	if(result != jsonData.size())
		return;
	tfile.commit();
}
