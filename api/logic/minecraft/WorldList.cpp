/* Copyright 2015 MultiMC Contributors
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

#include "WorldList.h"
#include <FileSystem.h>
#include <QMimeData>
#include <QUrl>
#include <QUuid>
#include <QString>
#include <QFileSystemWatcher>
#include <QDebug>

WorldList::WorldList(const QString &dir)
	: QAbstractListModel(), m_dir(dir)
{
	FS::ensureFolderPathExists(m_dir.absolutePath());
	m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs |
					QDir::NoSymLinks);
	m_dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
	m_watcher = new QFileSystemWatcher(this);
	is_watching = false;
	connect(m_watcher, SIGNAL(directoryChanged(QString)), this,
			SLOT(directoryChanged(QString)));
}

void WorldList::startWatching()
{
	if(is_watching)
	{
		return;
	}
	update();
	is_watching = m_watcher->addPath(m_dir.absolutePath());
	if (is_watching)
	{
		qDebug() << "Started watching " << m_dir.absolutePath();
	}
	else
	{
		qDebug() << "Failed to start watching " << m_dir.absolutePath();
	}
}

void WorldList::stopWatching()
{
	if(!is_watching)
	{
		return;
	}
	is_watching = !m_watcher->removePath(m_dir.absolutePath());
	if (!is_watching)
	{
		qDebug() << "Stopped watching " << m_dir.absolutePath();
	}
	else
	{
		qDebug() << "Failed to stop watching " << m_dir.absolutePath();
	}
}

bool WorldList::update()
{
	if (!isValid())
		return false;

	QList<World> newWorlds;
	m_dir.refresh();
	auto folderContents = m_dir.entryInfoList();
	// if there are any untracked files...
	for (QFileInfo entry : folderContents)
	{
		if(!entry.isDir())
			continue;

		World w(entry);
		if(w.isValid())
		{
			newWorlds.append(w);
		}
	}
	beginResetModel();
	worlds.swap(newWorlds);
	endResetModel();
	return true;
}

void WorldList::directoryChanged(QString path)
{
	update();
}

bool WorldList::isValid()
{
	return m_dir.exists() && m_dir.isReadable();
}

bool WorldList::deleteWorld(int index)
{
	if (index >= worlds.size() || index < 0)
		return false;
	World &m = worlds[index];
	if (m.destroy())
	{
		beginRemoveRows(QModelIndex(), index, index);
		worlds.removeAt(index);
		endRemoveRows();
		emit changed();
		return true;
	}
	return false;
}

bool WorldList::deleteWorlds(int first, int last)
{
	for (int i = first; i <= last; i++)
	{
		World &m = worlds[i];
		m.destroy();
	}
	beginRemoveRows(QModelIndex(), first, last);
	worlds.erase(worlds.begin() + first, worlds.begin() + last + 1);
	endRemoveRows();
	emit changed();
	return true;
}

int WorldList::columnCount(const QModelIndex &parent) const
{
	return 2;
}

QVariant WorldList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= worlds.size())
		return QVariant();

	auto & world = worlds[row];
	switch (role)
	{
	case Qt::DisplayRole:
		switch (column)
		{
		case NameColumn:
			return world.name();

		case LastPlayedColumn:
			return world.lastPlayed();

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
	{
		return world.folderName();
	}
	case ObjectRole:
	{
		return QVariant::fromValue<void *>((void *)&world);
	}
	case FolderRole:
	{
		return QDir::toNativeSeparators(dir().absoluteFilePath(world.folderName()));
	}
	case SeedRole:
	{
		return qVariantFromValue<qlonglong>(world.seed());
	}
	case NameRole:
	{
		return world.name();
	}
	case LastPlayedRole:
	{
		return world.lastPlayed();
	}
	default:
		return QVariant();
	}
}

QVariant WorldList::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		switch (section)
		{
		case NameColumn:
			return tr("Name");
		case LastPlayedColumn:
			return tr("Last Played");
		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		switch (section)
		{
		case NameColumn:
			return tr("The name of the world.");
		case LastPlayedColumn:
			return tr("Date and time the world was last played.");
		default:
			return QVariant();
		}
	default:
		return QVariant();
	}
	return QVariant();
}

QStringList WorldList::mimeTypes() const
{
	QStringList types;
	types << "text/uri-list";
	return types;
}

class WorldMimeData : public QMimeData
{
Q_OBJECT

public:
	WorldMimeData(QList<World> worlds)
	{
		m_worlds = worlds;

	}
	QStringList formats() const
	{
		return QMimeData::formats() << "text/uri-list";
	}

protected:
	QVariant retrieveData(const QString &mimetype, QVariant::Type type) const
	{
		QList<QUrl> urls;
		for(auto &world: m_worlds)
		{
			if(!world.isValid() || !world.isOnFS())
				continue;
			QString worldPath = world.container().absoluteFilePath();
			qDebug() << worldPath;
			urls.append(QUrl::fromLocalFile(worldPath));
		}
		const_cast<WorldMimeData*>(this)->setUrls(urls);
		return QMimeData::retrieveData(mimetype, type);
	}
private:
	QList<World> m_worlds;
};

QMimeData *WorldList::mimeData(const QModelIndexList &indexes) const
{
	if (indexes.size() == 0)
		return new QMimeData();

	QList<World> worlds;
	for(auto idx : indexes)
	{
		if(idx.column() != 0)
			continue;
		int row = idx.row();
		if (row < 0 || row >= this->worlds.size())
			continue;
		worlds.append(this->worlds[row]);
	}
	if(!worlds.size())
	{
		return new QMimeData();
	}
	return new WorldMimeData(worlds);
}

Qt::ItemFlags WorldList::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
	if (index.isValid())
		return Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |
			   defaultFlags;
	else
		return Qt::ItemIsDropEnabled | defaultFlags;
}

Qt::DropActions WorldList::supportedDragActions() const
{
	// move to other mod lists or VOID
	return Qt::MoveAction;
}

Qt::DropActions WorldList::supportedDropActions() const
{
	// copy from outside, move from within and other mod lists
	return Qt::CopyAction | Qt::MoveAction;
}

void WorldList::installWorld(QFileInfo filename)
{
	qDebug() << "installing: " << filename.absoluteFilePath();
	World w(filename);
	if(!w.isValid())
	{
		return;
	}
	w.install(m_dir.absolutePath());
}

bool WorldList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
							 const QModelIndex &parent)
{
	if (action == Qt::IgnoreAction)
		return true;
	// check if the action is supported
	if (!data || !(action & supportedDropActions()))
		return false;
	// files dropped from outside?
	if (data->hasUrls())
	{
		bool was_watching = is_watching;
		if (was_watching)
			stopWatching();
		auto urls = data->urls();
		for (auto url : urls)
		{
			// only local files may be dropped...
			if (!url.isLocalFile())
				continue;
			QString filename = url.toLocalFile();

			QFileInfo worldInfo(filename);

			if(!m_dir.entryInfoList().contains(worldInfo))
			{
				installWorld(worldInfo);
			}
		}
		if (was_watching)
			startWatching();
		return true;
	}
	return false;
}

#include "WorldList.moc"
