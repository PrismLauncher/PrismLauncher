/* Copyright 2013-2017 MultiMC Contributors
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

#include "IconList.h"
#include <FileSystem.h>
#include <QMap>
#include <QEventLoop>
#include <QMimeData>
#include <QUrl>
#include <QFileSystemWatcher>
#include <QSet>
#include <QDebug>

#define MAX_SIZE 1024

IconList::IconList(const QStringList &builtinPaths, QString path, QObject *parent) : QAbstractListModel(parent)
{
	QSet<QString> builtinNames;

	// add builtin icons
	for(auto & builtinPath: builtinPaths)
	{
		QDir instance_icons(builtinPath);
		auto file_info_list = instance_icons.entryInfoList(QDir::Files, QDir::Name);
		for (auto file_info : file_info_list)
		{
			builtinNames.insert(file_info.baseName());
		}
	}
	for(auto & builtinName : builtinNames)
	{
		addThemeIcon(builtinName);
	}

	m_watcher.reset(new QFileSystemWatcher());
	is_watching = false;
	connect(m_watcher.get(), SIGNAL(directoryChanged(QString)),
			SLOT(directoryChanged(QString)));
	connect(m_watcher.get(), SIGNAL(fileChanged(QString)), SLOT(fileChanged(QString)));

	directoryChanged(path);
}

void IconList::directoryChanged(const QString &path)
{
	QDir new_dir (path);
	if(m_dir.absolutePath() != new_dir.absolutePath())
	{
		m_dir.setPath(path);
		m_dir.refresh();
		if(is_watching)
			stopWatching();
		startWatching();
	}
	if(!m_dir.exists())
		if(!FS::ensureFolderPathExists(m_dir.absolutePath()))
			return;
	m_dir.refresh();
	auto new_list = m_dir.entryList(QDir::Files, QDir::Name);
	for (auto it = new_list.begin(); it != new_list.end(); it++)
	{
		QString &foo = (*it);
		foo = m_dir.filePath(foo);
	}
	auto new_set = new_list.toSet();
	QList<QString> current_list;
	for (auto &it : icons)
	{
		if (!it.has(IconType::FileBased))
			continue;
		current_list.push_back(it.m_images[IconType::FileBased].filename);
	}
	QSet<QString> current_set = current_list.toSet();

	QSet<QString> to_remove = current_set;
	to_remove -= new_set;

	QSet<QString> to_add = new_set;
	to_add -= current_set;

	for (auto remove : to_remove)
	{
		qDebug() << "Removing " << remove;
		QFileInfo rmfile(remove);
		QString key = rmfile.baseName();
		int idx = getIconIndex(key);
		if (idx == -1)
			continue;
		icons[idx].remove(IconType::FileBased);
		if (icons[idx].type() == IconType::ToBeDeleted)
		{
			beginRemoveRows(QModelIndex(), idx, idx);
			icons.remove(idx);
			reindex();
			endRemoveRows();
		}
		else
		{
			dataChanged(index(idx), index(idx));
		}
		m_watcher->removePath(remove);
		emit iconUpdated(key);
	}

	for (auto add : to_add)
	{
		qDebug() << "Adding " << add;
		QFileInfo addfile(add);
		QString key = addfile.baseName();
		if (addIcon(key, QString(), addfile.filePath(), IconType::FileBased))
		{
			m_watcher->addPath(add);
			emit iconUpdated(key);
		}
	}
}

void IconList::fileChanged(const QString &path)
{
	qDebug() << "Checking " << path;
	QFileInfo checkfile(path);
	if (!checkfile.exists())
		return;
	QString key = checkfile.baseName();
	int idx = getIconIndex(key);
	if (idx == -1)
		return;
	QIcon icon(path);
	if (!icon.availableSizes().size())
		return;

	icons[idx].m_images[IconType::FileBased].icon = icon;
	dataChanged(index(idx), index(idx));
	emit iconUpdated(key);
}

void IconList::SettingChanged(const Setting &setting, QVariant value)
{
	if(setting.id() != "IconsDir")
		return;

	directoryChanged(value.toString());
}

void IconList::startWatching()
{
	auto abs_path = m_dir.absolutePath();
	FS::ensureFolderPathExists(abs_path);
	is_watching = m_watcher->addPath(abs_path);
	if (is_watching)
	{
		qDebug() << "Started watching " << abs_path;
	}
	else
	{
		qDebug() << "Failed to start watching " << abs_path;
	}
}

void IconList::stopWatching()
{
	m_watcher->removePaths(m_watcher->files());
	m_watcher->removePaths(m_watcher->directories());
	is_watching = false;
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

bool IconList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
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
		auto urls = data->urls();
		QStringList iconFiles;
		for (auto url : urls)
		{
			// only local files may be dropped...
			if (!url.isLocalFile())
				continue;
			iconFiles += url.toLocalFile();
		}
		installIcons(iconFiles);
		return true;
	}
	return false;
}

Qt::ItemFlags IconList::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
	if (index.isValid())
		return Qt::ItemIsDropEnabled | defaultFlags;
	else
		return Qt::ItemIsDropEnabled | defaultFlags;
}

QVariant IconList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();

	if (row < 0 || row >= icons.size())
		return QVariant();

	switch (role)
	{
	case Qt::DecorationRole:
		return icons[row].icon();
	case Qt::DisplayRole:
		return icons[row].name();
	case Qt::UserRole:
		return icons[row].m_key;
	default:
		return QVariant();
	}
}

int IconList::rowCount(const QModelIndex &parent) const
{
	return icons.size();
}

void IconList::installIcons(const QStringList &iconFiles)
{
	for (QString file : iconFiles)
	{
		QFileInfo fileinfo(file);
		if (!fileinfo.isReadable() || !fileinfo.isFile())
			continue;
		QString target = FS::PathCombine(m_dir.dirName(), fileinfo.fileName());

		QString suffix = fileinfo.suffix();
		if (suffix != "jpeg" && suffix != "png" && suffix != "jpg" && suffix != "ico")
			continue;

		if (!QFile::copy(file, target))
			continue;
	}
}

bool IconList::iconFileExists(const QString &key) const
{
	auto iconEntry = icon(key);
	if(!iconEntry)
	{
		return false;
	}
	return iconEntry->has(IconType::FileBased);
}

const MMCIcon *IconList::icon(const QString &key) const
{
	int iconIdx = getIconIndex(key);
	if (iconIdx == -1)
		return nullptr;
	return &icons[iconIdx];
}

bool IconList::deleteIcon(const QString &key)
{
	int iconIdx = getIconIndex(key);
	if (iconIdx == -1)
		return false;
	auto &iconEntry = icons[iconIdx];
	if (iconEntry.has(IconType::FileBased))
	{
		return QFile::remove(iconEntry.m_images[IconType::FileBased].filename);
	}
	return false;
}

bool IconList::addThemeIcon(const QString& key)
{
	auto iter = name_index.find(key);
	if (iter != name_index.end())
	{
		auto &oldOne = icons[*iter];
		oldOne.replace(Builtin, key);
		dataChanged(index(*iter), index(*iter));
		return true;
	}
	else
	{
		// add a new icon
		beginInsertRows(QModelIndex(), icons.size(), icons.size());
		{
			MMCIcon mmc_icon;
			mmc_icon.m_name = key;
			mmc_icon.m_key = key;
			mmc_icon.replace(Builtin, key);
			icons.push_back(mmc_icon);
			name_index[key] = icons.size() - 1;
		}
		endInsertRows();
		return true;
	}
}

bool IconList::addIcon(const QString &key, const QString &name, const QString &path, const IconType type)
{
	// replace the icon even? is the input valid?
	QIcon icon(path);
	if (!icon.availableSizes().size())
		return false;
	auto iter = name_index.find(key);
	if (iter != name_index.end())
	{
		auto &oldOne = icons[*iter];
		oldOne.replace(type, icon, path);
		dataChanged(index(*iter), index(*iter));
		return true;
	}
	else
	{
		// add a new icon
		beginInsertRows(QModelIndex(), icons.size(), icons.size());
		{
			MMCIcon mmc_icon;
			mmc_icon.m_name = name;
			mmc_icon.m_key = key;
			mmc_icon.replace(type, icon, path);
			icons.push_back(mmc_icon);
			name_index[key] = icons.size() - 1;
		}
		endInsertRows();
		return true;
	}
}

void IconList::saveIcon(const QString &key, const QString &path, const char * format) const
{
	auto icon = getIcon(key);
	auto pixmap = icon.pixmap(128, 128);
	pixmap.save(path, format);
}


void IconList::reindex()
{
	name_index.clear();
	int i = 0;
	for (auto &iter : icons)
	{
		name_index[iter.m_key] = i;
		i++;
	}
}

QIcon IconList::getIcon(const QString &key) const
{
	int icon_index = getIconIndex(key);

	if (icon_index != -1)
		return icons[icon_index].icon();

	// Fallback for icons that don't exist.
	icon_index = getIconIndex("infinity");

	if (icon_index != -1)
		return icons[icon_index].icon();
	return QIcon();
}

QIcon IconList::getBigIcon(const QString &key) const
{
	int icon_index = getIconIndex(key);

	// Fallback for icons that don't exist.
	icon_index = getIconIndex(icon_index == -1 ? "infinity" : key);

	if (icon_index == -1)
		return QIcon();

	QPixmap bigone = icons[icon_index].icon().pixmap(256,256).scaled(256,256);
	return QIcon(bigone);
}

int IconList::getIconIndex(const QString &key) const
{
	auto iter = name_index.find(key == "default" ? "infinity" : key);
	if (iter != name_index.end())
		return *iter;

	return -1;
}

//#include "IconList.moc"
