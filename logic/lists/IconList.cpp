#include "IconList.h"
#include <pathutils.h>
#include <QMap>
#include <QEventLoop>
#include <QDir>
#include <QMimeData>
#include <QUrl>
#define MAX_SIZE 1024

struct entry
{
	QString key;
	QString name;
	QIcon icon;
	bool is_builtin;
	QString filename;
};

class Private : public QObject
{
	Q_OBJECT
public:
	QMap<QString, int> index;
	QVector<entry> icons;
	Private()
	{
	}
};


IconList::IconList() : QAbstractListModel(), d(new Private())
{
	QDir instance_icons(":/icons/instances/");
	auto file_info_list = instance_icons.entryInfoList(QDir::Files, QDir::Name);
	for(auto file_info: file_info_list)
	{
		QString key = file_info.baseName();
		addIcon(key, key, file_info.absoluteFilePath(), true);
	}
	
	// FIXME: get from settings
	ensureFolderPathExists("icons");
	QDir user_icons("icons");
	file_info_list = user_icons.entryInfoList(QDir::Files, QDir::Name);
	for(auto file_info: file_info_list)
	{
		QString filename = file_info.absoluteFilePath();
		QString key = file_info.baseName();
		addIcon(key, key, filename);
	}
}

IconList::~IconList()
{
	delete d;
	d = nullptr;
}

QStringList IconList::mimeTypes() const
{
	QStringList types;
	types << "text/uri-list";
	return types;
}
Qt::DropActions IconList::supportedDropActions() const
{
	return Qt::CopyAction;
}

bool IconList::dropMimeData ( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
	if (action == Qt::IgnoreAction)
        return true;
	// check if the action is supported
	if (!data || !(action & supportedDropActions()))
		return false;

	// files dropped from outside?
	if(data->hasUrls())
	{
		/*
		bool was_watching = is_watching;
		if(was_watching)
			stopWatching();
		*/
		auto urls = data->urls();
		QStringList iconFiles;
		for(auto url: urls)
		{
			// only local files may be dropped...
			if(!url.isLocalFile())
				continue;
			iconFiles += url.toLocalFile();
		}
		installIcons(iconFiles);
		/*
		if(was_watching)
			startWatching();
		*/
		return true;
	}
	return false;
}

Qt::ItemFlags IconList::flags ( const QModelIndex& index ) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags ( index );
	if (index.isValid())
		return Qt::ItemIsDropEnabled | defaultFlags;
	else
		return Qt::ItemIsDropEnabled | defaultFlags;
}

QVariant IconList::data ( const QModelIndex& index, int role ) const
{
	if(!index.isValid())
		return QVariant();
	
	int row = index.row();
	
	if(row < 0 || row >= d->icons.size())
		return QVariant();
	
	switch(role)
	{
		case Qt::DecorationRole:
			return d->icons[row].icon;
		case Qt::DisplayRole:
			return d->icons[row].name;
		case Qt::UserRole:
			return d->icons[row].key;
		default:
			return QVariant();
	}
}

int IconList::rowCount ( const QModelIndex& parent ) const
{
	return d->icons.size();
}

void IconList::installIcons ( QStringList iconFiles )
{
	for(QString file: iconFiles)
	{
		QFileInfo fileinfo(file);
		if(!fileinfo.isReadable() || !fileinfo.isFile())
			continue;
		QString target = PathCombine("icons", fileinfo.fileName());
		
		QString suffix = fileinfo.suffix();
		if(suffix != "jpeg" && suffix != "png" && suffix != "jpg")
			continue;
		
		if(!QFile::copy(file, target))
			continue;
		
		QString key = fileinfo.baseName();
		addIcon(key, key, target);
	}
}

bool IconList::deleteIcon ( QString key )
{
	int iconIdx = getIconIndex(key);
	if(iconIdx == -1)
		return false;
	auto & iconEntry = d->icons[iconIdx];
	if(iconEntry.is_builtin)
		return false;
	if(QFile::remove(iconEntry.filename))
	{
		beginRemoveRows(QModelIndex(), iconIdx, iconIdx);
		d->icons.remove(iconIdx);
		reindex();
		endRemoveRows();
	}
	return true;
}

bool IconList::addIcon ( QString key, QString name, QString path, bool is_builtin )
{
	auto iter = d->index.find(key);
	if(iter != d->index.end())
	{
		if(d->icons[*iter].is_builtin)
			return false;
		
		QIcon icon(path);
		if(icon.isNull())
			return false;
		
		auto & oldOne = d->icons[*iter];
		
		if(!QFile::remove(oldOne.filename))
			return false;

		// replace the icon
		oldOne = {key, name, icon, is_builtin, path};
		dataChanged(index(*iter),index(*iter));
		return true;
	}
	else
	{
		QIcon icon(path);
		if(icon.isNull()) return false;
		
		// add a new icon
		beginInsertRows(QModelIndex(), d->icons.size(),d->icons.size());
		d->icons.push_back({key, name, icon, is_builtin, path});
		d->index[key] = d->icons.size() - 1;
		endInsertRows();
		return true;
	}
}

void IconList::reindex()
{
	d->index.clear();
	int i = 0;
	for(auto& iter: d->icons)
	{
		d->index[iter.key] = i;
		i++;
	}
}


QIcon IconList::getIcon ( QString key )
{
	int icon_index = getIconIndex(key);

	if(icon_index != -1)
		return d->icons[icon_index].icon;
	
	// Fallback for icons that don't exist.
	icon_index = getIconIndex("infinity");
	
	if(icon_index != -1)
		return d->icons[icon_index].icon;
	return QIcon();
}

int IconList::getIconIndex ( QString key )
{
	if(key == "default")
		key = "infinity";
	
	auto iter = d->index.find(key);
	if(iter != d->index.end())
		return *iter;
	
	
	return -1;
}

#include "IconList.moc"