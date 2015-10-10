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

#include <QDir>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <quazip.h>
#include <quazipfile.h>

#include "Mod.h"
#include "settings/INIFile.h"
#include <FileSystem.h>
#include <QDebug>

Mod::Mod(const QFileInfo &file)
{
	repath(file);
}

void Mod::repath(const QFileInfo &file)
{
	m_file = file;
	QString name_base = file.fileName();

	m_type = Mod::MOD_UNKNOWN;

	if (m_file.isDir())
	{
		m_type = MOD_FOLDER;
		m_name = name_base;
		m_mmc_id = name_base;
	}
	else if (m_file.isFile())
	{
		if (name_base.endsWith(".disabled"))
		{
			m_enabled = false;
			name_base.chop(9);
		}
		else
		{
			m_enabled = true;
		}
		m_mmc_id = name_base;
		if (name_base.endsWith(".zip") || name_base.endsWith(".jar"))
		{
			m_type = MOD_ZIPFILE;
			name_base.chop(4);
		}
		else if (name_base.endsWith(".litemod"))
		{
			m_type = MOD_LITEMOD;
			name_base.chop(8);
		}
		else
		{
			m_type = MOD_SINGLEFILE;
		}
		m_name = name_base;
	}

	if (m_type == MOD_ZIPFILE)
	{
		QuaZip zip(m_file.filePath());
		if (!zip.open(QuaZip::mdUnzip))
			return;

		QuaZipFile file(&zip);

		if (zip.setCurrentFile("mcmod.info"))
		{
			if (!file.open(QIODevice::ReadOnly))
			{
				zip.close();
				return;
			}

			ReadMCModInfo(file.readAll());
			file.close();
			zip.close();
			return;
		}
		else if (zip.setCurrentFile("forgeversion.properties"))
		{
			if (!file.open(QIODevice::ReadOnly))
			{
				zip.close();
				return;
			}

			ReadForgeInfo(file.readAll());
			file.close();
			zip.close();
			return;
		}

		zip.close();
	}
	else if (m_type == MOD_FOLDER)
	{
		QFileInfo mcmod_info(FS::PathCombine(m_file.filePath(), "mcmod.info"));
		if (mcmod_info.isFile())
		{
			QFile mcmod(mcmod_info.filePath());
			if (!mcmod.open(QIODevice::ReadOnly))
				return;
			auto data = mcmod.readAll();
			if (data.isEmpty() || data.isNull())
				return;
			ReadMCModInfo(data);
		}
	}
	else if (m_type == MOD_LITEMOD)
	{
		QuaZip zip(m_file.filePath());
		if (!zip.open(QuaZip::mdUnzip))
			return;

		QuaZipFile file(&zip);

		if (zip.setCurrentFile("litemod.json"))
		{
			if (!file.open(QIODevice::ReadOnly))
			{
				zip.close();
				return;
			}

			ReadLiteModInfo(file.readAll());
			file.close();
		}
		zip.close();
	}
}

// NEW format
// https://github.com/MinecraftForge/FML/wiki/FML-mod-information-file/6f62b37cea040daf350dc253eae6326dd9c822c3

// OLD format:
// https://github.com/MinecraftForge/FML/wiki/FML-mod-information-file/5bf6a2d05145ec79387acc0d45c958642fb049fc
void Mod::ReadMCModInfo(QByteArray contents)
{
	auto getInfoFromArray = [&](QJsonArray arr)->void
	{
		if (!arr.at(0).isObject())
			return;
		auto firstObj = arr.at(0).toObject();
		m_mod_id = firstObj.value("modid").toString();
		m_name = firstObj.value("name").toString();
		m_version = firstObj.value("version").toString();
		m_homeurl = firstObj.value("url").toString();
		m_updateurl = firstObj.value("updateUrl").toString();
		m_homeurl = m_homeurl.trimmed();
		if(!m_homeurl.isEmpty())
		{
			// fix up url.
			if (!m_homeurl.startsWith("http://") && !m_homeurl.startsWith("https://") &&
				!m_homeurl.startsWith("ftp://"))
			{
				m_homeurl.prepend("http://");
			}
		}
		m_description = firstObj.value("description").toString();
		QJsonArray authors = firstObj.value("authorList").toArray();
		if (authors.size() == 0)
			authors = firstObj.value("authors").toArray();

		if (authors.size() == 0)
			m_authors = "";
		else if (authors.size() >= 1)
		{
			m_authors = authors.at(0).toString();
			for (int i = 1; i < authors.size(); i++)
			{
				m_authors += ", " + authors.at(i).toString();
			}
		}
		m_credits = firstObj.value("credits").toString();
		return;
	}
	;
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(contents, &jsonError);
	// this is the very old format that had just the array
	if (jsonDoc.isArray())
	{
		getInfoFromArray(jsonDoc.array());
	}
	else if (jsonDoc.isObject())
	{
		auto val = jsonDoc.object().value("modinfoversion");
		if(val.isUndefined())
			val = jsonDoc.object().value("modListVersion");
		int version = val.toDouble();
		if (version != 2)
		{
			qCritical() << "BAD stuff happened to mod json:";
			qCritical() << contents;
			return;
		}
		auto arrVal = jsonDoc.object().value("modlist");
		if(arrVal.isUndefined())
			arrVal = jsonDoc.object().value("modList");
		if (arrVal.isArray())
		{
			getInfoFromArray(arrVal.toArray());
		}
	}
}

void Mod::ReadForgeInfo(QByteArray contents)
{
	// Read the data
	m_name = "Minecraft Forge";
	m_mod_id = "Forge";
	m_homeurl = "http://www.minecraftforge.net/forum/";
	INIFile ini;
	if (!ini.loadFile(contents))
		return;

	QString major = ini.get("forge.major.number", "0").toString();
	QString minor = ini.get("forge.minor.number", "0").toString();
	QString revision = ini.get("forge.revision.number", "0").toString();
	QString build = ini.get("forge.build.number", "0").toString();

	m_version = major + "." + minor + "." + revision + "." + build;
}

void Mod::ReadLiteModInfo(QByteArray contents)
{
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(contents, &jsonError);
	auto object = jsonDoc.object();
	if (object.contains("name"))
	{
		m_mod_id = m_name = object.value("name").toString();
	}
	if (object.contains("version"))
	{
		m_version = object.value("version").toString("");
	}
	else
	{
		m_version = object.value("revision").toString("");
	}
	m_mcversion = object.value("mcversion").toString();
	m_authors = object.value("author").toString();
	m_description = object.value("description").toString();
	m_homeurl = object.value("url").toString();
}

bool Mod::replace(Mod &with)
{
	if (!destroy())
		return false;
	bool success = false;
	auto t = with.type();

	if (t == MOD_ZIPFILE || t == MOD_SINGLEFILE || t == MOD_LITEMOD)
	{
		qDebug() << "Copy: " << with.m_file.filePath() << " to " << m_file.filePath();
		success = QFile::copy(with.m_file.filePath(), m_file.filePath());
	}
	if (t == MOD_FOLDER)
	{
		success = FS::copy(with.m_file.filePath(), m_file.path())();
	}
	if (success)
	{
		m_name = with.m_name;
		m_mmc_id = with.m_mmc_id;
		m_mod_id = with.m_mod_id;
		m_version = with.m_version;
		m_mcversion = with.m_mcversion;
		m_description = with.m_description;
		m_authors = with.m_authors;
		m_credits = with.m_credits;
		m_homeurl = with.m_homeurl;
		m_type = with.m_type;
		m_file.refresh();
	}
	return success;
}

bool Mod::destroy()
{
	if (m_type == MOD_FOLDER)
	{
		QDir d(m_file.filePath());
		if (d.removeRecursively())
		{
			m_type = MOD_UNKNOWN;
			return true;
		}
		return false;
	}
	else if (m_type == MOD_SINGLEFILE || m_type == MOD_ZIPFILE || m_type == MOD_LITEMOD)
	{
		QFile f(m_file.filePath());
		if (f.remove())
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
	switch (type())
	{
	case MOD_ZIPFILE:
	case MOD_LITEMOD:
		return m_version;
	case MOD_FOLDER:
		return "Folder";
	case MOD_SINGLEFILE:
		return "File";
	default:
		return "VOID";
	}
}

bool Mod::enable(bool value)
{
	if (m_type == Mod::MOD_UNKNOWN || m_type == Mod::MOD_FOLDER)
		return false;

	if (m_enabled == value)
		return false;

	QString path = m_file.absoluteFilePath();
	if (value)
	{
		QFile foo(path);
		if (!path.endsWith(".disabled"))
			return false;
		path.chop(9);
		if (!foo.rename(path))
			return false;
	}
	else
	{
		QFile foo(path);
		path += ".disabled";
		if (!foo.rename(path))
			return false;
	}
	m_file = QFileInfo(path);
	m_enabled = value;
	return true;
}
bool Mod::operator==(const Mod &other) const
{
	return mmc_id() == other.mmc_id();
}
bool Mod::strongCompare(const Mod &other) const
{
	return mmc_id() == other.mmc_id() && version() == other.version() && type() == other.type();
}
