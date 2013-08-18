// 
//  Copyright 2013 MultiMC Contributors
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#include "ModList.h"
#include "LegacyInstance.h"
#include <pathutils.h>
#include <QMimeData>
#include <QUrl>
#include <QDebug>
#include <QUuid>

ModList::ModList ( const QString& dir, const QString& list_file )
: QAbstractListModel(), m_dir(dir), m_list_file(list_file)
{
	m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs | QDir::NoSymLinks);
	m_dir.setSorting(QDir::Name);
	m_list_id = QUuid::createUuid().toString();
	update();
}

bool ModList::update()
{
	if (!isValid())
		return false;
	
	QList<Mod> newMods;
	
	auto folderContents = m_dir.entryInfoList();
	bool orderWasInvalid = false;
	
	// first, process the ordered items (if any)
	int currentOrderIndex = 0;
	QStringList listOrder = readListFile();
	for(auto item: listOrder)
	{
		QFileInfo info (m_dir.filePath(item));
		int idx = folderContents.indexOf(info);
		// if the file from the index file exists
		if(idx != -1)
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
	for(auto entry: folderContents)
	{
		newMods.append(Mod(entry));
	}
	if(mods.size() != newMods.size())
	{
		orderWasInvalid = true;
	}
	else for(int i = 0; i < mods.size(); i++)
	{
		if(!mods[i].strongCompare(newMods[i]))
		{
			orderWasInvalid = true;
			break;
		}
	}
	beginResetModel();
	mods.swap(newMods);
	endResetModel();
	if(orderWasInvalid)
	{
		saveListFile();
		emit changed();
	}
	return true;
}

QStringList ModList::readListFile()
{
	QStringList stringList;
	if(m_list_file.isNull() || m_list_file.isEmpty())
		return stringList;
	
	QFile textFile(m_list_file);
	if(!textFile.open(QIODevice::ReadOnly | QIODevice::Text))
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
	if(m_list_file.isNull() || m_list_file.isEmpty())
		return false;
	QFile textFile(m_list_file);
	if(!textFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		return false;
	QTextStream textStream(&textFile);
	for(auto mod:mods)
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

bool ModList::installMod ( const QFileInfo& filename, size_t index )
{
	if(!filename.exists() || !filename.isReadable())
	{
		return false;
	}
	Mod m(filename);
	if(!m.valid())
		return false;
	
	// if it's already there, replace the original mod (in place)
	int idx = mods.indexOf(m);
	if(idx != -1)
	{
		if(mods[idx].replace(m))
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
	if(type == Mod::MOD_UNKNOWN)
		return false;
	if(type == Mod::MOD_SINGLEFILE || type == Mod::MOD_ZIPFILE)
	{
		QString newpath = PathCombine(m_dir.path(), filename.fileName());
		if(!QFile::copy(filename.filePath(), newpath))
			return false;
		m.repath(newpath);
		beginInsertRows(QModelIndex(), index, index);
		mods.insert(index,m);
		endInsertRows();
		saveListFile();
		emit changed();
		return true;
	}
	else if(type == Mod::MOD_FOLDER)
	{
		
		QString from = filename.filePath();
		QString to = PathCombine(m_dir.path(), filename.fileName());
		if(!copyPath(from, to))
			return false;
		m.repath(to);
		beginInsertRows(QModelIndex(), index, index);
		mods.insert(index,m);
		endInsertRows();
		saveListFile();
		emit changed();
		return true;
	}
	return false;
}

bool ModList::deleteMod ( size_t index )
{
	if(index >= mods.size())
		return false;
	Mod & m = mods[index];
	if(m.destroy())
	{
		beginRemoveRows(QModelIndex(), index, index);
		mods.erase(mods.begin() + index);
		endRemoveRows();
		saveListFile();
		emit changed();
		return true;
	}
	return false;
}

bool ModList::moveMod ( size_t from, size_t to )
{
	if(from < 0 || from >= mods.size())
		return false;
	if (to >= rowCount())
		to = rowCount() - 1;
	if (to == -1)
		to = rowCount() - 1;
	// FIXME: this should be better, but segfaults for some reason
	//beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
	
	beginResetModel();
	mods.move(from, to);
	endResetModel();
	//endMoveRows();
	saveListFile();
	emit changed();
	return true;
}

int ModList::columnCount ( const QModelIndex& parent ) const
{
	return 2;
}

QVariant ModList::data ( const QModelIndex& index, int role ) const
{
	if(!index.isValid())
		return QVariant();
	
	int row = index.row();
	int column = index.column();
	
	if(row < 0 || row >= mods.size())
		return QVariant();
	
	if(role != Qt::DisplayRole)
		return QVariant();
	
	switch(column)
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

QVariant ModList::headerData ( int section, Qt::Orientation orientation, int role ) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();
	switch (section)
	{
	case 0:
		return QString("Mod Name");
	case 1:
		return QString("Mod Version");
	case 2:
		return QString("MC Version");
	}
}


Qt::ItemFlags ModList::flags ( const QModelIndex& index ) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags ( index );
	if (index.isValid())
		return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
	else
		return Qt::ItemIsDropEnabled | defaultFlags;
}

QStringList ModList::mimeTypes() const
{
	QStringList types;
	types << "text/uri-list";
	types << "application/x-mcmod";
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

QMimeData* ModList::mimeData ( const QModelIndexList& indexes ) const
{
	if(indexes.size() == 0)
		return nullptr;
	
	auto idx = indexes[0];
	int row = idx.row();
	if(row <0 || row >= mods.size())
		return nullptr;
	
	QMimeData * data = new QMimeData();
	
	QStringList params;
	params << m_list_id << QString::number(row);
	data->setData("application/x-mcmod", params.join('|').toLatin1());
	return data;
}

bool ModList::dropMimeData ( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
	if (action == Qt::IgnoreAction)
        return true;
	// check if the action is supported
	if (!data || !(action & supportedDropActions()))
		return false;
	qDebug() << "row: " << row << " column: " << column;
	if(parent.isValid())
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
	qDebug() << "row: " << row << " column: " << column;
	
	// files dropped from outside?
	if(data->hasFormat("text/uri-list") && data->hasUrls())
	{
		auto urls = data->urls();
		for(auto url: urls)
		{
			// only local files may be dropped...
			if(!url.isLocalFile())
				continue;
			QString filename = url.toLocalFile();
			installMod(filename, row);
			qDebug() << "installing: " << filename;
		}
		return true;
	}
	else if(data->hasFormat("application/x-mcmod"))
	{
		QString sourcestr = QString::fromLatin1(data->data("application/x-mcmod"));
		auto list = sourcestr.split('|');
		if(list.size() != 2)
			return false;
		QString remoteId = list[0];
		int remoteIndex = list[1].toInt();
		// no moving of things between two lists
		if(remoteId != m_list_id)
			return false;
		// no point moving to the same place...
		if(row == remoteIndex)
			return false;
		// otherwise, move the mod :D
		moveMod(remoteIndex, row);
		return true;
	}
	return false;
}


/*
ModList::ModList(const QString &dir)
	: modsFolder(dir)
{
	
}

bool ModList::update(bool quickLoad)
{
	bool listChanged = false;

	// Check for mods in the list whose files do not exist and remove them from the list.
	// If doing a quickLoad, erase the whole list.
	for (size_t i = 0; i < size(); i++)
	{
		if (quickLoad || !at(i).GetFileName().FileExists())
		{
			erase(begin() + i);
			i--;
			listChanged = true;
		}
	}

	// Add any mods in the mods folder that aren't already in the list.
	if (LoadModListFromDir(QString(), quickLoad))
		listChanged = true;

	return listChanged;
}

bool ModList::LoadModListFromDir(const QString& loadFrom, bool quickLoad)
{
	QString dir(loadFrom.isEmpty() ? modsFolder : loadFrom);

	QDir modDir(dir);
	if (!modDir.exists())
		return false;

	bool listChanged = false;

	auto list = modDir.entryInfoList(QDir::Readable|QDir::NoDotAndDotDot, QDir::Name);
	for(auto currentFile: list)
	{
		if (currentFile.isFile())
		{
			if (quickLoad || FindByFilename(currentFile.absoluteFilePath()) == nullptr)
			{
				Mod mod(currentFile.absoluteFilePath());
				push_back(mod);
				listChanged = true;
			}
		}
		else if (currentFile.isDir())
		{
			if (LoadModListFromDir(currentFile.absoluteFilePath()))
				listChanged = true;
		}
	}

	return listChanged;
}

Mod *ModList::FindByFilename(const QString& filename)
{
	// Search the list for a mod with the given filename.
	for (auto iter = begin(); iter != end(); ++iter)
	{
		if (iter->GetFileName() == QFileInfo(filename))
			return &(*iter);
	}

	// If nothing is found, return nullptr.
	return nullptr;
}

int ModList::FindIndexByFilename(const QString& filename)
{
	// Search the list for a mod with the given filename.
	int i = 0;
	for (auto iter = begin(); iter != end(); ++iter, i++)
	{
		if (iter->GetFileName() == QFileInfo(filename))
			return i;
	}

	// If nothing is found, return nullptr.
	return -1;
}

Mod* ModList::FindByID(const QString& modID, const QString& modVersion)
{
	// Search the list for a mod that matches
	for (auto iter = begin(); iter != end(); ++iter)
	{
		QString ID = iter->GetModID();
		QString version = iter->GetModVersion();
		if ( ID == modID && version == modVersion)
			return &(*iter);
	}

	// If nothing is found, return nullptr.
	return nullptr;
}

void ModList::LoadFromFile(const QString& file)
{
	if (!wxFileExists(file))
		return;

	wxFFileInputStream inputStream(file);
	wxArrayString modListFile = ReadAllLines(inputStream);

	for (wxArrayString::iterator iter = modListFile.begin(); iter != modListFile.end(); iter++)
	{
		// Normalize the path to the instMods dir.
		wxFileName modFile(*iter);
		modFile.Normalize(wxPATH_NORM_ALL, modsFolder);
		modFile.MakeRelativeTo();
		// if the file is gone, do not load it
		if(!modFile.Exists())
		{
			continue;
		}

		if (FindByFilename(modFile.GetFullPath()) == nullptr)
		{
			push_back(Mod(modFile));
		}
	}
}

void ModList::SaveToFile(const QString& file)
{
	QString text;
	for (iterator iter = begin(); iter != end(); ++iter)
	{
		wxFileName modFile = iter->GetFileName();
		modFile.MakeRelativeTo(modsFolder);
		text.append(modFile.GetFullPath());
		text.append("\n");
	}

	wxTempFileOutputStream out(file);
	WriteAllText(out, text);
	out.Commit();
}

bool ModList::InsertMod(size_t index, const QString &filename, const QString& saveToFile)
{
	QFileInfo source(filename);
	QFileInfo dest(PathCombine(modsFolder, source.fileName()));

	if (source != dest)
	{
		QFile::copy(source.absoluteFilePath(), dest.absoluteFilePath());
	}

	int oldIndex = FindIndexByFilename(dest.absoluteFilePath());

	if (oldIndex != -1)
	{
		erase(begin() + oldIndex);
	}

	if (index >= size())
		push_back(Mod(dest));
	else
		insert(begin() + index, Mod(dest));

	if (!saveToFile.isEmpty())
		SaveToFile(saveToFile);

	return true;
}

bool ModList::DeleteMod(size_t index, const QString& saveToFile)
{
	Mod *mod = &at(index);
	if(mod->GetModType() == Mod::MOD_FOLDER)
	{
		QDir dir(mod->GetFileName().absoluteFilePath());
		if(dir.removeRecursively())
		{
			erase(begin() + index);
			
			if (!saveToFile.isEmpty())
				SaveToFile(saveToFile);
			
			return true;
		}
		else
		{
			// wxLogError(_("Failed to delete mod."));
		}
	}
	else if (QFile(mod->GetFileName().absoluteFilePath()).remove())
	{
		erase(begin() + index);
		
		if (!saveToFile.isEmpty())
			SaveToFile(saveToFile);

		return true;
	}
	else
	{
		// wxLogError(_("Failed to delete mod."));
	}
	return false;
}

bool JarModList::InsertMod(size_t index, const QString &filename, const QString& saveToFile)
{
	QString saveFile = saveToFile;
	if (saveToFile.isEmpty())
		saveFile = m_inst->GetModListFile().GetFullPath();

	if (ModList::InsertMod(index, filename, saveFile))
	{
		m_inst->setLWJGLVersion(true);
		return true;
	}
	return false;
}

bool JarModList::DeleteMod(size_t index, const QString& saveToFile)
{
	QString saveFile = saveToFile;
	if (saveToFile.IsEmpty())
		saveFile = m_inst->GetModListFile().GetFullPath();

	if (ModList::DeleteMod(index, saveFile))
	{
		m_inst->SetNeedsRebuild();
		return true;
	}
	return false;
}

bool JarModList::UpdateModList(bool quickLoad)
{
	if (ModList::UpdateModList(quickLoad))
	{
		m_inst->SetNeedsRebuild();
		return true;
	}
	return false;
}

bool FolderModList::LoadModListFromDir(const QString& loadFrom, bool quickLoad)
{
	QString dir(loadFrom.IsEmpty() ? modsFolder : loadFrom);

	if (!wxDirExists(dir))
		return false;

	bool listChanged = false;
	wxDir modDir(dir);

	if (!modDir.IsOpened())
	{
		wxLogError(_("Failed to open directory: ") + dir);
		return false;
	}

	QString currentFile;
	if (modDir.GetFirst(&currentFile))
	{
		do
		{
			wxFileName modFile(Path::Combine(dir, currentFile));

			if (wxFileExists(modFile.GetFullPath()) || wxDirExists(modFile.GetFullPath()))
			{
				if (quickLoad || FindByFilename(modFile.GetFullPath()) == nullptr)
				{
					Mod mod(modFile.GetFullPath());
					push_back(mod);
					listChanged = true;
				}
			}
		} while (modDir.GetNext(&currentFile));
	}

	return listChanged;
}

bool ModNameSort (const Mod & i,const Mod & j)
{
	if(i.GetModType() == j.GetModType())
		return (i.GetName().toLower() < j.GetName().toLower());
	return (i.GetModType() < j.GetModType());
}

bool FolderModList::UpdateModList ( bool quickLoad )
{
	bool changed = ModList::UpdateModList(quickLoad);
	std::sort(begin(),end(),ModNameSort);
	return changed;
}
*/
