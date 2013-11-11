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

#include <QDir>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <quazip.h>
#include <quazipfile.h>

#include "Mod.h"
#include <pathutils.h>
#include <inifile.h>
#include "logger/QsLog.h"

Mod::Mod(const QFileInfo &file)
{
	repath(file);
}

void Mod::repath(const QFileInfo &file)
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
	if (m_type == MOD_ZIPFILE)
	{
		QuaZip zip(m_file.filePath());
		if (!zip.open(QuaZip::mdUnzip))
			return;

		QuaZipFile file(&zip);

		if (zip.setCurrentFile("mcmod.info"))
		{
			if(!file.open(QIODevice::ReadOnly))
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
		QFileInfo mcmod_info(PathCombine(m_file.filePath(), "mcmod.info"));
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
		m_id = firstObj.value("modid").toString();
		m_name = firstObj.value("name").toString();
		m_version = firstObj.value("version").toString();
		m_homeurl = firstObj.value("url").toString();
		m_description = firstObj.value("description").toString();
		QJsonArray authors = firstObj.value("authors").toArray();
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
		int version = val.toDouble();
		if (version != 2)
		{
			QLOG_ERROR() << "BAD stuff happened to mod json:";
			QLOG_ERROR() << contents;
			return;
		}
		auto arrVal = jsonDoc.object().value("modlist");
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
	m_id = "Forge";
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

bool Mod::replace(Mod &with)
{
	if (!destroy())
		return false;
	bool success = false;
	auto t = with.type();
	if (t == MOD_ZIPFILE || t == MOD_SINGLEFILE)
	{
		success = QFile::copy(with.m_file.filePath(), m_file.path());
	}
	if (t == MOD_FOLDER)
	{
		success = copyPath(with.m_file.filePath(), m_file.path());
	}
	if (success)
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
	else if (m_type == MOD_SINGLEFILE || m_type == MOD_ZIPFILE)
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
		return m_version;
	case MOD_FOLDER:
		return "Folder";
	case MOD_SINGLEFILE:
		return "File";
	default:
		return "VOID";
	}
}
