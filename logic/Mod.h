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

#pragma once
#include <QFileInfo>

class Mod
{
public:
	enum ModType
	{
		MOD_UNKNOWN,	//!< Indicates an unspecified mod type.
		MOD_ZIPFILE,	//!< The mod is a zip file containing the mod's class files.
		MOD_SINGLEFILE, //!< The mod is a single file (not a zip file).
		MOD_FOLDER,	 //!< The mod is in a folder on the filesystem.
	};

	Mod(const QFileInfo &file);

	QFileInfo filename() const
	{
		return m_file;
	}
	QString id() const
	{
		return m_id;
	}
	ModType type() const
	{
		return m_type;
	}
	QString mcversion() const
	{
		return m_mcversion;
	}
	;
	bool valid()
	{
		return m_type != MOD_UNKNOWN;
	}
	QString name() const
	{
		return m_name;
	}

	QString version() const;

	QString homeurl() const
	{
		return m_homeurl;
	}

	QString description() const
	{
		return m_description;
	}

	QString authors() const
	{
		return m_authors;
	}

	QString credits() const
	{
		return m_credits;
	}

	// delete all the files of this mod
	bool destroy();
	// replace this mod with a copy of the other
	bool replace(Mod &with);
	// change the mod's filesystem path (used by mod lists for *MAGIC* purposes)
	void repath(const QFileInfo &file);

	// WEAK compare operator - used for replacing mods
	bool operator==(const Mod &other) const
	{
		return filename() == other.filename();
	}
	bool strongCompare(const Mod &other) const
	{
		return filename() == other.filename() && id() == other.id() &&
			   version() == other.version() && type() == other.type();
	}

private:
	void ReadMCModInfo(QByteArray contents);
	void ReadForgeInfo(QByteArray contents);

protected:

	// FIXME: what do do with those? HMM...
	/*
	void ReadModInfoData(QString info);
	void ReadForgeInfoData(QString infoFileData);
	*/

	QFileInfo m_file;
	QString m_id;
	QString m_name;
	QString m_version;
	QString m_mcversion;
	QString m_homeurl;
	QString m_description;
	QString m_authors;
	QString m_credits;

	ModType m_type;
};
