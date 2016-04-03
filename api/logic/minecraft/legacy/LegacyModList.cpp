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

#include "LegacyModList.h"
#include <FileSystem.h>
#include <QMimeData>
#include <QUrl>
#include <QUuid>
#include <QString>
#include <QFileSystemWatcher>
#include <QDebug>

LegacyModList::LegacyModList(const QString &dir, const QString &list_file)
	: QAbstractListModel(), m_dir(dir), m_list_file(list_file)
{
	FS::ensureFolderPathExists(m_dir.absolutePath());
	m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs |
					QDir::NoSymLinks);
	m_dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
	m_list_id = QUuid::createUuid().toString();
	m_watcher = new QFileSystemWatcher(this);
	is_watching = false;
	connect(m_watcher, SIGNAL(directoryChanged(QString)), this,
			SLOT(directoryChanged(QString)));
}

void LegacyModList::startWatching()
{
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

void LegacyModList::stopWatching()
{
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

void LegacyModList::internalSort(QList<Mod> &what)
{
	auto predicate = [](const Mod &left, const Mod &right)
	{
		if (left.name() == right.name())
		{
			return left.mmc_id().localeAwareCompare(right.mmc_id()) < 0;
		}
		return left.name().localeAwareCompare(right.name()) < 0;
	};
	std::sort(what.begin(), what.end(), predicate);
}

bool LegacyModList::update()
{
	if (!isValid())
		return false;

	QList<Mod> orderedMods;
	QList<Mod> newMods;
	m_dir.refresh();
	auto folderContents = m_dir.entryInfoList();
	bool orderOrStateChanged = false;

	// first, process the ordered items (if any)
	OrderList listOrder = readListFile();
	for (auto item : listOrder)
	{
		QFileInfo infoEnabled(m_dir.filePath(item.id));
		QFileInfo infoDisabled(m_dir.filePath(item.id + ".disabled"));
		int idxEnabled = folderContents.indexOf(infoEnabled);
		int idxDisabled = folderContents.indexOf(infoDisabled);
		bool isEnabled;
		// if both enabled and disabled versions are present, it's a special case...
		if (idxEnabled >= 0 && idxDisabled >= 0)
		{
			// we only process the one we actually have in the order file.
			// and exactly as we have it.
			// THIS IS A CORNER CASE
			isEnabled = item.enabled;
		}
		else
		{
			// only one is present.
			// we pick the one that we found.
			// we assume the mod was enabled/disabled by external means
			isEnabled = idxEnabled >= 0;
		}
		int idx = isEnabled ? idxEnabled : idxDisabled;
		QFileInfo &info = isEnabled ? infoEnabled : infoDisabled;
		// if the file from the index file exists
		if (idx != -1)
		{
			// remove from the actual folder contents list
			folderContents.takeAt(idx);
			// append the new mod
			orderedMods.append(Mod(info));
			if (isEnabled != item.enabled)
				orderOrStateChanged = true;
		}
		else
		{
			orderOrStateChanged = true;
		}
	}
	// if there are any untracked files...
	if (folderContents.size())
	{
		// the order surely changed!
		for (auto entry : folderContents)
		{
			newMods.append(Mod(entry));
		}
		internalSort(newMods);
		orderedMods.append(newMods);
		orderOrStateChanged = true;
	}
	// otherwise, if we were already tracking some mods
	else if (mods.size())
	{
		// if the number doesn't match, order changed.
		if (mods.size() != orderedMods.size())
			orderOrStateChanged = true;
		// if it does match, compare the mods themselves
		else
			for (int i = 0; i < mods.size(); i++)
			{
				if (!mods[i].strongCompare(orderedMods[i]))
				{
					orderOrStateChanged = true;
					break;
				}
			}
	}
	beginResetModel();
	mods.swap(orderedMods);
	endResetModel();
	if (orderOrStateChanged && !m_list_file.isEmpty())
	{
		qDebug() << "Mod list " << m_list_file << " changed!";
		saveListFile();
		emit changed();
	}
	return true;
}

void LegacyModList::directoryChanged(QString path)
{
	update();
}

LegacyModList::OrderList LegacyModList::readListFile()
{
	OrderList itemList;
	if (m_list_file.isNull() || m_list_file.isEmpty())
		return itemList;

	QFile textFile(m_list_file);
	if (!textFile.open(QIODevice::ReadOnly | QIODevice::Text))
		return OrderList();

	QTextStream textStream;
	textStream.setAutoDetectUnicode(true);
	textStream.setDevice(&textFile);
	while (true)
	{
		QString line = textStream.readLine();
		if (line.isNull() || line.isEmpty())
			break;
		else
		{
			OrderItem it;
			it.enabled = !line.endsWith(".disabled");
			if (!it.enabled)
			{
				line.chop(9);
			}
			it.id = line;
			itemList.append(it);
		}
	}
	textFile.close();
	return itemList;
}

bool LegacyModList::saveListFile()
{
	if (m_list_file.isNull() || m_list_file.isEmpty())
		return false;
	QFile textFile(m_list_file);
	if (!textFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		return false;
	QTextStream textStream;
	textStream.setGenerateByteOrderMark(true);
	textStream.setCodec("UTF-8");
	textStream.setDevice(&textFile);
	for (auto mod : mods)
	{
		textStream << mod.mmc_id();
		if (!mod.enabled())
			textStream << ".disabled";
		textStream << endl;
	}
	textFile.close();
	return false;
}

bool LegacyModList::isValid()
{
	return m_dir.exists() && m_dir.isReadable();
}

bool LegacyModList::installMod(const QString &filename, int index)
{
	// NOTE: fix for GH-1178: remove trailing slash to avoid issues with using the empty result of QFileInfo::fileName
	QFileInfo fileinfo(FS::NormalizePath(filename));

	qDebug() << "installing: " << fileinfo.absoluteFilePath();

	if (!fileinfo.exists() || !fileinfo.isReadable() || index < 0)
	{
		return false;
	}
	Mod m(fileinfo);
	if (!m.valid())
		return false;

	// if it's already there, replace the original mod (in place)
	int idx = mods.indexOf(m);
	if (idx != -1)
	{
		int idx2 = mods.indexOf(m, idx + 1);
		if (idx2 != -1)
			return false;
		if (mods[idx].replace(m))
		{

			auto left = this->index(index);
			auto right = this->index(index, columnCount(QModelIndex()) - 1);
			emit dataChanged(left, right);
			saveListFile();
			update();
			return true;
		}
		return false;
	}

	auto type = m.type();
	if (type == Mod::MOD_UNKNOWN)
		return false;
	if (type == Mod::MOD_SINGLEFILE || type == Mod::MOD_ZIPFILE || type == Mod::MOD_LITEMOD)
	{
		QString newpath = FS::PathCombine(m_dir.path(), fileinfo.fileName());
		if (!QFile::copy(fileinfo.filePath(), newpath))
			return false;
		m.repath(newpath);
		beginInsertRows(QModelIndex(), index, index);
		mods.insert(index, m);
		endInsertRows();
		saveListFile();
		update();
		return true;
	}
	else if (type == Mod::MOD_FOLDER)
	{

		QString from = fileinfo.filePath();
		QString to = FS::PathCombine(m_dir.path(), fileinfo.fileName());
		if (!FS::copy(from, to)())
			return false;
		m.repath(to);
		beginInsertRows(QModelIndex(), index, index);
		mods.insert(index, m);
		endInsertRows();
		saveListFile();
		update();
		return true;
	}
	return false;
}

bool LegacyModList::deleteMod(int index)
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

bool LegacyModList::deleteMods(int first, int last)
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

bool LegacyModList::moveModTo(int from, int to)
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

bool LegacyModList::moveModUp(int from)
{
	if (from > 0)
		return moveModTo(from, from - 1);
	return false;
}

bool LegacyModList::moveModsUp(int first, int last)
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

bool LegacyModList::moveModDown(int from)
{
	if (from < 0)
		return false;
	if (from < mods.size() - 1)
		return moveModTo(from, from + 1);
	return false;
}

bool LegacyModList::moveModsDown(int first, int last)
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

int LegacyModList::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QVariant LegacyModList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= mods.size())
		return QVariant();

	switch (role)
	{
	case Qt::DisplayRole:
		switch (column)
		{
		case NameColumn:
			return mods[row].name();
		case VersionColumn:
			return mods[row].version();

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		return mods[row].mmc_id();

	case Qt::CheckStateRole:
		switch (column)
		{
		case ActiveColumn:
			return mods[row].enabled() ? Qt::Checked : Qt::Unchecked;
		default:
			return QVariant();
		}
	default:
		return QVariant();
	}
}

bool LegacyModList::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
	{
		return false;
	}

	if (role == Qt::CheckStateRole)
	{
		auto &mod = mods[index.row()];
		if (mod.enable(!mod.enabled()))
		{
			emit dataChanged(index, index);
			return true;
		}
	}
	return false;
}

QVariant LegacyModList::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		switch (section)
		{
		case ActiveColumn:
			return QString();
		case NameColumn:
			return tr("Name");
		case VersionColumn:
			return tr("Version");
		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		switch (section)
		{
		case ActiveColumn:
			return tr("Is the mod enabled?");
		case NameColumn:
			return tr("The name of the mod.");
		case VersionColumn:
			return tr("The version of the mod.");
		default:
			return QVariant();
		}
	default:
		return QVariant();
	}
	return QVariant();
}

Qt::ItemFlags LegacyModList::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
	if (index.isValid())
		return Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |
			   defaultFlags;
	else
		return Qt::ItemIsDropEnabled | defaultFlags;
}

QStringList LegacyModList::mimeTypes() const
{
	QStringList types;
	types << "text/uri-list";
	types << "text/plain";
	return types;
}

Qt::DropActions LegacyModList::supportedDropActions() const
{
	// copy from outside, move from within and other mod lists
	return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions LegacyModList::supportedDragActions() const
{
	// move to other mod lists or VOID
	return Qt::MoveAction;
}

QMimeData *LegacyModList::mimeData(const QModelIndexList &indexes) const
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

bool LegacyModList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
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
	qDebug() << "Drop row: " << row << " column: " << column;

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
			// if there is no ordering, re-sort the list
			if (m_list_file.isEmpty())
			{
				beginResetModel();
				internalSort(mods);
				endResetModel();
			}
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
		qDebug() << "move: " << sourcestr;
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
