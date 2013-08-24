#include "IconListModel.h"
#include <pathutils.h>
#include <QMap>
#include <QEventLoop>
#include <QDir>

#define MAX_SIZE 1024
IconList* IconList::m_Instance = 0;
QMutex IconList::mutex;

struct entry
{
	QString key;
	QString name;
	QIcon icon;
	bool is_builtin;
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

bool IconList::addIcon ( QString key, QString name, QString path, bool is_builtin )
{
	auto iter = d->index.find(key);
	if(iter != d->index.end())
	{
		if(d->icons[*iter].is_builtin) return false;
		
		QIcon icon(path);
		if(icon.isNull()) return false;
		
		// replace the icon
		d->icons[*iter] = {key, name, icon, is_builtin};
		return true;
	}
	else
	{
		QIcon icon(path);
		if(icon.isNull()) return false;
		
		// add a new icon
		d->icons.push_back({key, name, icon, is_builtin});
		d->index[key] = d->icons.size() - 1;
		return true;
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


void IconList::drop()
{
	mutex.lock();
	delete m_Instance;
	m_Instance = 0;
	mutex.unlock();
}

IconList* IconList::instance()
{
	if ( !m_Instance )
	{
		mutex.lock();
		if ( !m_Instance )
			m_Instance = new IconList;
		mutex.unlock();
	}
	return m_Instance;
}

#include "IconListModel.moc"