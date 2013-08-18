// 
//  Copyright 2012 MultiMC Contributors
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

#include "Mod.h"
#include <pathutils.h>
#include <QDir>

Mod::Mod( const QFileInfo& file )
{
	repath(file);
}

void Mod::repath ( const QFileInfo& file )
{
	m_file = file;
	m_name = file.completeBaseName();
	m_id = file.fileName();
	
	m_type = Mod::MOD_UNKNOWN;
	if (m_file.isDir())
		m_type = MOD_FOLDER;
	else if (m_file.isFile())
	{
		QString ext = m_file.suffix().toLower();
		if (ext == "zip" || ext == "jar")
			m_type = MOD_ZIPFILE;
		else
			m_type = MOD_SINGLEFILE;
	}

	/*
	switch (modType)
	{
	case MOD_ZIPFILE:
		{
			wxFFileInputStream fileIn(modFile.GetFullPath());
			wxZipInputStream zipIn(fileIn);

			std::auto_ptr<wxZipEntry> entry;

			bool is_forge = false;
			while(true)
			{
				entry.reset(zipIn.GetNextEntry());
				if (entry.get() == nullptr)
					break;
				if(entry->GetInternalName().EndsWith("mcmod.info"))
					break;
				if(entry->GetInternalName().EndsWith("forgeversion.properties"))
				{
					is_forge = true;
					break;
				}
			}

			if (entry.get() != nullptr)
			{
				// Read the info file into text
				wxString infoFileData;
				wxStringOutputStream stringOut(&infoFileData);
				zipIn.Read(stringOut);
				if(!is_forge)
					ReadModInfoData(infoFileData);
				else
					ReadForgeInfoData(infoFileData);
			}
		}
		break;

	case MOD_FOLDER:
		{
			wxString infoFile = Path::Combine(modFile, "mcmod.info");
			if (!wxFileExists(infoFile))
			{
				infoFile = wxEmptyString;

				wxDir modDir(modFile.GetFullPath());

				if (!modDir.IsOpened())
				{
					wxLogError(_("Can't fine mod info file. Failed to open mod folder."));
					break;
				}

				wxString currentFile;
				if (modDir.GetFirst(&currentFile))
				{
					do 
					{
						if (currentFile.EndsWith("mcmod.info"))
						{
							infoFile = Path::Combine(modFile.GetFullPath(), currentFile);
							break;
						}
					} while (modDir.GetNext(&currentFile));
				}
			}

			if (infoFile != wxEmptyString && wxFileExists(infoFile))
			{
				wxString infoStr;
				wxFFileInputStream fileIn(infoFile);
				wxStringOutputStream strOut(&infoStr);
				fileIn.Read(strOut);
				ReadModInfoData(infoStr);
			}
		}
		break;
	}
*/
}


/*
void ReadModInfoData(QString info)
{
	using namespace boost::property_tree;

	// Read the data
	ptree ptRoot;

	std::stringstream stringIn(cStr(info));
	try
	{
		read_json(stringIn, ptRoot);

		ptree pt = ptRoot.get_child(ptRoot.count("modlist") == 1 ? "modlist" : "").begin()->second;

		modID = wxStr(pt.get<std::string>("modid"));
		modName = wxStr(pt.get<std::string>("name"));
		modVersion = wxStr(pt.get<std::string>("version"));
	}
	catch (json_parser_error e)
	{
		// Silently fail...
	}
	catch (ptree_error e)
	{
		// Silently fail...
	}
}
*/

// FIXME: abstraction violated.
/*
void Mod::ReadForgeInfoData(QString infoFileData)
{
		using namespace boost::property_tree;

	// Read the data
	ptree ptRoot;
	modName = "Minecraft Forge";
	modID = "Forge";
	std::stringstream stringIn(cStr(infoFileData));
	try
	{
		read_ini(stringIn, ptRoot);
		wxString major, minor, revision, build;
		// BUG: boost property tree is bad. won't let us get a key with dots in it
		// Likely cause = treating the dots as path separators.
		for (auto iter = ptRoot.begin(); iter != ptRoot.end(); iter++)
		{
			auto &item = *iter;
			std::string key = item.first;
			std::string value = item.second.get_value<std::string>();
			if(key == "forge.major.number")
				major = value;
			if(key == "forge.minor.number")
				minor = value;
			if(key == "forge.revision.number")
				revision = value;
			if(key == "forge.build.number")
				build = value;
		}
		modVersion.Empty();
		modVersion << major << "." << minor << "." << revision << "." << build;
	}
	catch (json_parser_error e)
	{
		std::cerr << e.what();
	}
	catch (ptree_error e)
	{
		std::cerr << e.what();
	}
}
*/

bool Mod::replace ( Mod& with )
{
	if(!destroy())
		return false;
	bool success = false;
	auto t = with.type();
	if(t == MOD_ZIPFILE || t == MOD_SINGLEFILE)
	{
		success = QFile::copy(with.m_file.filePath(), m_file.path());
	}
	if(t == MOD_FOLDER)
	{
		success = copyPath(with.m_file.filePath(), m_file.path());
	}
	if(success)
	{
		m_id = with.m_id;
		m_mcversion = with.m_mcversion;
		m_type = with.m_type;
		m_name = with.m_name;
		m_version = with.m_version;
	}
	return success;
}

bool Mod::destroy()
{
	if(m_type == MOD_FOLDER)
	{
		QDir d(m_file.filePath());
		if(d.removeRecursively())
		{
			m_type = MOD_UNKNOWN;
			return true;
		}
		return false;
	}
	else if (m_type == MOD_SINGLEFILE || m_type == MOD_ZIPFILE)
	{
		QFile f(m_file.filePath());
		if(f.remove())
		{
			m_type = MOD_UNKNOWN;
			return true;
		}
		return false;
	}
	return true;
}


QString Mod::version() const
{
	switch(type())
	{
		case MOD_ZIPFILE:
			return m_version;
		case MOD_FOLDER:
			return "Folder";
		case MOD_SINGLEFILE:
			return "File";
	}
}
