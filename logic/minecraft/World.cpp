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

#include <QDir>
#include <QString>
#include <QDebug>
#include "World.h"
#include <pathutils.h>

#include "GZip.h"
#include <MMCZip.h>
#include <sstream>
#include <io/stream_reader.h>
#include <tag_string.h>
#include <tag_primitive.h>
#include <quazip.h>
#include <quazipfile.h>

World::World(const QFileInfo &file)
{
	repath(file);
}

void World::repath(const QFileInfo &file)
{
	m_containerFile = file;
	m_folderName = file.fileName();
	if(file.isFile() && file.suffix() == "zip")
	{
		readFromZip(file);
	}
	else if(file.isDir())
	{
		readFromFS(file);
	}
}

void World::readFromFS(const QFileInfo &file)
{
	QDir worldDir(file.filePath());
	is_valid = file.isDir() && worldDir.exists("level.dat");
	if(!is_valid)
	{
		return;
	}

	auto fullFilePath = worldDir.absoluteFilePath("level.dat");
	QFile f(fullFilePath);
	is_valid = f.open(QIODevice::ReadOnly);
	if(!is_valid)
	{
		return;
	}
	QFileInfo finfo(fullFilePath);
	levelDatTime = finfo.lastModified();
	parseLevelDat(f.readAll());
}

void World::readFromZip(const QFileInfo &file)
{
	QuaZip zip(file.absoluteFilePath());
	is_valid = zip.open(QuaZip::mdUnzip);
	if (!is_valid)
	{
		return;
	}
	QuaZipFile zippedFile(&zip);
	// read the install profile
	is_valid = zip.setCurrentFile("level.dat");
	if (!is_valid)
	{
		return;
	}
	is_valid = zippedFile.open(QIODevice::ReadOnly);
	QuaZipFileInfo64 levelDatInfo;
	zippedFile.getFileInfo(&levelDatInfo);
	auto modTime = levelDatInfo.getNTFSmTime();
	if(!modTime.isValid())
	{
		modTime = levelDatInfo.dateTime;
	}
	levelDatTime = modTime;
	if (!is_valid)
	{
		return;
	}
	parseLevelDat(zippedFile.readAll());
	zippedFile.close();
}

bool World::install(QString to)
{
	auto finalPath = PathCombine(to, DirNameFromString(m_actualName, to));
	if(!ensureFolderPathExists(finalPath))
	{
		return false;
	}
	if(m_containerFile.isFile())
	{
		// FIXME: check if this is OK.
		return !MMCZip::extractDir(m_containerFile.absoluteFilePath(), finalPath).isEmpty();
	}
	else if(m_containerFile.isDir())
	{
		QString from = m_containerFile.filePath();
		return copyPath(from, finalPath);
	}
	return false;
}

static QString read_string (nbt::value& parent, const char * name, const QString & fallback = QString())
{
	try
	{
		auto &namedValue = parent.at(name);
		if(namedValue.get_type() != nbt::tag_type::String)
		{
			return fallback;
		}
		auto & tag_str = namedValue.as<nbt::tag_string>();
		return QString::fromStdString(tag_str.get());
	}
	catch(std::out_of_range e)
	{
		// fallback for old world formats
		qWarning() << "String NBT tag" << name << "could not be found. Defaulting to" << fallback;
		return fallback;
	}
	catch(std::bad_cast e)
	{
		// type mismatch
		qWarning() << "NBT tag" << name << "could not be converted to string. Defaulting to" << fallback;
		return fallback;
	}
};

static int64_t read_long (nbt::value& parent, const char * name, const int64_t & fallback = 0)
{
	try
	{
		auto &namedValue = parent.at(name);
		if(namedValue.get_type() != nbt::tag_type::Long)
		{
			return fallback;
		}
		auto & tag_str = namedValue.as<nbt::tag_long>();
		return tag_str.get();
	}
	catch(std::out_of_range e)
	{
		// fallback for old world formats
		qWarning() << "Long NBT tag" << name << "could not be found. Defaulting to" << fallback;
		return fallback;
	}
	catch(std::bad_cast e)
	{
		// type mismatch
		qWarning() << "NBT tag" << name << "could not be converted to long. Defaulting to" << fallback;
		return fallback;
	}
};

void World::parseLevelDat(QByteArray data)
{
	QByteArray output;
	is_valid = GZip::inflate(data, output);
	if(!is_valid)
	{
		return;
	}

	try
	{
		std::istringstream foo(std::string(output.constData(), output.size()));
		auto pair = nbt::io::read_compound(foo);
		is_valid  = pair.first == "";

		if(!is_valid)
		{
			return;
		}
		std::ostringstream ostr;
		is_valid = pair.second != nullptr;
		if(!is_valid)
		{
			qDebug() << "FAIL~!!!";
			return;
		}

		auto &val = pair.second->at("Data");
		is_valid = val.get_type() == nbt::tag_type::Compound;
		if(!is_valid)
			return;

		m_actualName = read_string(val, "LevelName", m_folderName);


		int64_t temp = read_long(val, "LastPlayed", 0);
		if(temp == 0)
		{
			m_lastPlayed = levelDatTime;
		}
		else
		{
			m_lastPlayed = QDateTime::fromMSecsSinceEpoch(temp);
		}

		m_randomSeed = read_long(val, "RandomSeed", 0);

		qDebug() << "World Name:" << m_actualName;
		qDebug() << "Last Played:" << m_lastPlayed.toString();
		qDebug() << "Seed:" << m_randomSeed;
	}
	catch (nbt::io::input_error e)
	{
		qWarning() << "Unable to load" << m_folderName << ":" << e.what();
		is_valid = false;
		return;
	}
}

bool World::replace(World &with)
{
	if (!destroy())
		return false;
	bool success = copyPath(with.m_containerFile.filePath(), m_containerFile.path());
	if (success)
	{
		m_folderName = with.m_folderName;
		m_containerFile.refresh();
	}
	return success;
}

bool World::destroy()
{
	if(!is_valid) return false;
	if (m_containerFile.isDir())
	{
		QDir d(m_containerFile.filePath());
		return d.removeRecursively();
	}
	else if(m_containerFile.isFile())
	{
		QFile file(m_containerFile.absoluteFilePath());
		return file.remove();
	}
	return true;
}

bool World::operator==(const World &other) const
{
	return is_valid == other.is_valid && folderName() == other.folderName();
}
bool World::strongCompare(const World &other) const
{
	return is_valid == other.is_valid && folderName() == other.folderName();
}
