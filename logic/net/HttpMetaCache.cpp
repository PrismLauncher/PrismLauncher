#include "HttpMetaCache.h"
#include <pathutils.h>
#include <QFile>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qfileinfo.h>
#include <qtemporaryfile.h>
#include <qsavefile.h>

HttpMetaCache::HttpMetaCache(QString path)
{
	m_index_file = path;
}
HttpMetaCache::~HttpMetaCache()
{
	Save();
}

void HttpMetaCache::addEntry ( QString base, QString resource_path, QString etag )
{
	// no base. no base path. can't store
	if(!m_entries.contains(base))
		return;
	QString real_path = PathCombine(m_entries[base].base_path, resource_path);
	QFileInfo finfo(real_path);
	
	// just ignore it, it's garbage if it's not a proper file
	if(!finfo.isFile() || !finfo.isReadable())
	{
		// TODO: log problem
		return;
	}
	
	Save();
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
	if(!version_val.isArray())
		return;
	QJsonArray array = json.array();
	for(auto element: array)
	{
		if(!element.isObject());
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
		foo->last_changed_timestamp = element_obj.value("last_changed_timestamp").toDouble();
		entrymap.entry_list[path] = MetaEntryPtr( foo );
	}
}

void HttpMetaCache::Save()
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
			entryObj.insert("last_changed_timestamp", QJsonValue(double(entry->last_changed_timestamp)));
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


MetaEntryPtr HttpMetaCache::getEntryForResource ( QString base, QString resource_path )
{
	if(!m_entries.contains(base))
		return MetaEntryPtr();
	auto & entrymap = m_entries[base];
	if(!entrymap.entry_list.contains(resource_path))
		return MetaEntryPtr();
	return entrymap.entry_list[resource_path];
}
