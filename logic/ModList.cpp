/* Copyright 2013 MultiMC Contributors
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

#include "ModList.h"
#include "LegacyInstance.h"
#include <pathutils.h>
#include <QMimeData>
#include <QUrl>
#include <QUuid>
#include <QFileSystemWatcher>
#include "logger/QsLog.h"

ModList::ModList(const QString &dir, const QString &list_file)
	: QAbstractListModel(), m_dir(dir), m_list_file(list_file)
{
	m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs |
					QDir::NoSymLinks);
	m_dir.setSorting(QDir::Name);
	m_list_id = QUuid::createUuid().toString();
	m_watcher = new QFileSystemWatcher(this);
	is_watching = false;
	connect(m_watcher, SIGNAL(directoryChanged(QString)), this,
			SLOT(directoryChanged(QString)));
}

void ModList::startWatching()
{
	is_watching = m_watcher->addPath(m_dir.absolutePath());
	if (is_watching)
		QLOG_INFO() << "Started watching " << m_dir.absolutePath();
	else
		QLOG_INFO() << "Failed to start watching " << m_dir.absolutePath();
}

void ModList::stopWatching()
{
	is_watching = !m_watcher->removePath(m_dir.absolutePath());
	if (!is_watching)
		QLOG_INFO() << "Stopped watching " << m_dir.absolutePath();
	else
		QLOG_INFO() << "Failed to stop watching " << m_dir.absolutePath();
}

bool ModList::update()
{
	if (!isValid())
		return false;

	QList<Mod> newMods;
	m_dir.refresh();
	auto folderContents = m_dir.entryInfoList();
	bool orderWasInvalid = false;

	// first, process the ordered items (if any)
	int currentOrderIndex = 0;
	QStringList listOrder = readListFile();
	for (auto item : listOrder)
	{
		QFileInfo info(m_dir.filePath(item));
		int idx = folderContents.indexOf(info);
		// if the file from the index file exists
		if (idx != -1)
		{
			// remove from the actual folder contents list
			folderContents.takeAt(idx);
			// append the new mod
			newMods.append(Mod(info));
		}
		else
		{
			orderWasInvalid = true;
		}
	}
	for (auto entry : folderContents)
	{
		newMods.append(Mod(entry));
	}
	if (mods.size() != newMods.size())
	{
		orderWasInvalid = true;
	}
	else
		for (int i = 0; i < mods.size(); i++)
		{
			if (!mods[i].strongCompare(newMods[i]))
			{
				orderWasInvalid = true;
				break;
			}
		}
	beginResetModel();
	mods.swap(newMods);
	endResetModel();
	if (orderWasInvalid)
	{
		saveListFile();
		emit changed();
	}
	return true;
}

void ModList::directoryChanged(QString path)
{
	update();
}

QStringList ModList::readListFile()
{
	QStringList stringList;
	if (m_list_file.isNull() || m_list_file.isEmpty())
		return stringList;

	QFile textFile(m_list_file);
	if (!textFile.open(QIODevice::ReadOnly | QIODevice::Text))
		return QStringList();

	QTextStream textStream(&textFile);
	while (true)
	{
		QString line = textStream.readLine();
		if (line.isNull() || line.isEmpty())
			break;
		else
		{
			stringList.append(line);
		}
	}
	textFile.close();
	return stringList;
}

bool ModList::saveListFile()
{
	if (m_list_file.isNull() || m_list_file.isEmpty())
		return false;
	QFile textFile(m_list_file);
	if (!textFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		return false;
	QTextStream textStream(&textFile);
	for (auto mod : mods)
	{
		auto pathname = mod.filename();
		QString filename = pathname.fileName();
		textStream << filename << endl;
	}
	textFile.close();
	return false;
}

bool ModList::isValid()
{
	return m_dir.exists() && m_dir.isReadable();
}

bool ModList::installMod(const QFileInfo &filename, int index)
{
	if (!filename.exists() || !filename.isReadable() || index < 0)
	{
		return false;
	}
	Mod m(filename);
	if (!m.valid())
		return false;

	// if it's already there, replace the original mod (in place)
	int idx = mods.indexOf(m);
	if (idx != -1)
	{
		if (mods[idx].replace(m))
		{

			auto left = this->index(index);
			auto right = this->index(index, columnCount(QModelIndex()) - 1);
			emit dataChanged(left, right);
			saveListFile();
			emit changed();
			return true;
		}
		return false;
	}

	auto type = m.type();
	if (type == Mod::MOD_UNKNOWN)
		return false;
	if (type == Mod::MOD_SINGLEFILE || type == Mod::MOD_ZIPFILE)
	{
		QString newpath = PathCombine(m_dir.path(), filename.fileName());
		if (!QFile::copy(filename.filePath(), newpath))
			return false;
		m.repath(newpath);
		beginInsertRows(QModelIndex(), index, index);
		mods.insert(index, m);
		endInsertRows();
		saveListFile();
		emit changed();
		return true;
	}
	else if (type == Mod::MOD_FOLDER)
	{

		QString from = filename.filePath();
		QString to = PathCombine(m_dir.path(), filename.fileName());
		if (!copyPath(from, to))
			return false;
		m.repath(to);
		beginInsertRows(QModelIndex(), index, index);
		mods.insert(index, m);
		endInsertRows();
		saveListFile();
		emit changed();
		return true;
	}
	return false;
}

bool ModList::deleteMod(int index)
{
	if (index >= mods.size() || index < 0)
		return false;
	Mod &m = mods[index];
	if (m.destroy())
	{
		beginRemoveRows(QModelIndex(), index, index);
		mods.removeAt(index);
		endRemoveRows();
		saveListFile();
		emit changed();
		return true;
	}
	return false;
}

bool ModList::deleteMods(int first, int last)
{
	for (int i = first; i <= last; i++)
	{
		Mod &m = mods[i];
		m.destroy();
	}
	beginRemoveRows(QModelIndex(), first, last);
	mods.erase(mods.begin() + first, mods.begin() + last + 1);
	endRemoveRows();
	saveListFile();
	emit changed();
	return true;
}

bool ModList::moveModTo(int from, int to)
{
	if (from < 0 || from >= mods.size())
		return false;
	if (to >= rowCount())
		to = rowCount() - 1;
	if (to == -1)
		to = rowCount() - 1;
	if (from == to)
		return false;
	int togap = to > from ? to + 1 : to;
	beginMoveRows(QModelIndex(), from, from, QModelIndex(), togap);
	mods.move(from, to);
	endMoveRows();
	saveListFile();
	emit changed();
	return true;
}

bool ModList::moveModUp(int from)
{
	if (from > 0)
		return moveModTo(from, from - 1);
	return false;
}

bool ModList::moveModsUp(int first, int last)
{
	if (first == 0)
		return false;

	beginMoveRows(QModelIndex(), first, last, QModelIndex(), first - 1);
	mods.move(first - 1, last);
	endMoveRows();
	saveListFile();
	emit changed();
	return true;
}

bool ModList::moveModDown(int from)
{
	if (from < 0)
		return false;
	if (from < mods.size() - 1)
		return moveModTo(from, from + 1);
	return false;
}

bool ModList::moveModsDown(int first, int last)
{
	if (last == mods.size() - 1)
		return false;

	beginMoveRows(QModelIndex(), first, last, QModelIndex(), last + 2);
	mods.move(last + 1, first);
	endMoveRows();
	saveListFile();
	emit changed();
	return true;
}

int ModList::columnCount(const QModelIndex &parent) const
{
	return 2;
}

QVariant ModList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= mods.size())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	switch (column)
	{
	case 0:
		return mods[row].name();
	case 1:
		return mods[row].version();
	case 2:
		return mods[row].mcversion();
	default:
		return QVariant();
	}
}

QVariant ModList::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();
	switch (section)
	{
	case 0:
		return QString("Name");
	case 1:
		return QString("Version");
	case 2:
		return QString("Minecraft");
	}
}

Qt::ItemFlags ModList::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
	if (index.isValid())
		return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
	else
		return Qt::ItemIsDropEnabled | defaultFlags;
}

QStringList ModList::mimeTypes() const
{
	QStringList types;
	types << "text/uri-list";
	types << "text/plain";
	return types;
}

Qt::DropActions ModList::supportedDropActions() const
{
	// copy from outside, move from within and other mod lists
	return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions ModList::supportedDragActions() const
{
	// move to other mod lists or VOID
	return Qt::MoveAction;
}

QMimeData *ModList::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *data = new QMimeData();

	if (indexes.size() == 0)
		return data;

	auto idx = indexes[0];
	int row = idx.row();
	if (row < 0 || row >= mods.size())
		return data;

	QStringList params;
	params << m_list_id << QString::number(row);
	data->setText(params.join('|'));
	return data;
}
bool ModList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
						   const QModelIndex &parent)
{
	if (action == Qt::IgnoreAction)
		return true;
	// check if the action is supported
	if (!data || !(action & supportedDropActions()))
		return false;
	if (parent.isValid())
	{
		row = parent.row();
		column = parent.column();
	}

	if (row > rowCount())
		row = rowCount();
	if (row == -1)
		row = rowCount();
	if (column == -1)
		column = 0;
	QLOG_INFO() << "Drop row: " << row << " column: " << column;

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
			installMod(filename, row);
			QLOG_INFO() << "installing: " << filename;
		}
		if (was_watching)
			startWatching();
		return true;
	}
	else if (data->hasText())
	{
		QString sourcestr = data->text();
		auto list = sourcestr.split('|');
		if (list.size() != 2)
			return false;
		QString remoteId = list[0];
		int remoteIndex = list[1].toInt();
		QLOG_INFO() << "move: " << sourcestr;
		// no moving of things between two lists
		if (remoteId != m_list_id)
			return false;
		// no point moving to the same place...
		if (row == remoteIndex)
			return false;
		// otherwise, move the mod :D
		moveModTo(remoteIndex, row);
		return true;
	}
	return false;
}
