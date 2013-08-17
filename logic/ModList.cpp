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

ModList::ModList ( const QString& dir, const QString& list_file )
: QAbstractListModel(), m_dir(dir), m_list_file(list_file)
{
	m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs | QDir::NoSymLinks);
	m_dir.setSorting(QDir::Name);
	update();
}

bool ModList::update()
{
	if (!isValid())
		return false;

	bool initial = mods.empty();

	bool listChanged = false;

	auto list = m_dir.entryInfoList();
	for(auto entry: list)
	{
		Mod mod(entry);
		if (initial || !mods.contains(mod))
		{
			mods.push_back(mod);
			listChanged = true;
		}
	}
	return listChanged;
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
		mods.append(m);
		emit changed();
		return true;
	}
	else if(type == Mod::MOD_FOLDER)
	{
		QString newpath = PathCombine(m_dir.path(), filename.fileName());
		if(!copyPath(filename.filePath(), newpath))
			return false;
		m.repath(newpath);
		mods.append(m);
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
		mods.erase(mods.begin() + index);
		emit changed();
		return true;
	}
	return false;
}

bool ModList::moveMod ( size_t from, size_t to )
{
	return false;
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
