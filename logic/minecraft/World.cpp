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
#include <QSaveFile>
#include "World.h"

#include "GZip.h"
#include <MMCZip.h>
#include <FileSystem.h>
#include <sstream>
#include <io/stream_reader.h>
#include <tag_string.h>
#include <tag_primitive.h>
#include <quazip.h>
#include <quazipfile.h>
#include <quazipdir.h>

std::unique_ptr <nbt::tag_compound> parseLevelDat(QByteArray data)
{
	QByteArray output;
	if(!GZip::unzip(data, output))
	{
		return nullptr;
	}
	std::istringstream foo(std::string(output.constData(), output.size()));
	auto pair = nbt::io::read_compound(foo);

	if(pair.first != "")
		return nullptr;

	if(pair.second == nullptr)
		return nullptr;

	return std::move(pair.second);
}

QByteArray serializeLevelDat(nbt::tag_compound * levelInfo)
{
	std::ostringstream s;
	nbt::io::write_tag("", *levelInfo, s);
	QByteArray val( s.str().data(), (int) s.str().size() );
	return val;
}

QString getLevelDatFromFS(const QFileInfo &file)
{
	QDir worldDir(file.filePath());
	if(!file.isDir() || !worldDir.exists("level.dat"))
	{
		return QString();
	}
	return worldDir.absoluteFilePath("level.dat");
}

QByteArray getLevelDatDataFromFS(const QFileInfo &file)
{
	auto fullFilePath = getLevelDatFromFS(file);
	if(fullFilePath.isNull())
	{
		return QByteArray();
	}
	QFile f(fullFilePath);
	if(!f.open(QIODevice::ReadOnly))
	{
		return QByteArray();
	}
	return f.readAll();
}

bool putLevelDatDataToFS(const QFileInfo &file, QByteArray & data)
{
	auto fullFilePath =  getLevelDatFromFS(file);
	if(fullFilePath.isNull())
	{
		return false;
	}
	QSaveFile f(fullFilePath);
	if(!f.open(QIODevice::WriteOnly))
	{
		return false;
	}
	QByteArray compressed;
	if(!GZip::zip(data, compressed))
	{
		return false;
	}
	if(f.write(compressed) != compressed.size())
	{
		f.cancelWriting();
		return false;
	}
	return f.commit();
}

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
	auto bytes = getLevelDatDataFromFS(file);
	if(bytes.isEmpty())
	{
		is_valid = false;
		return;
	}
	loadFromLevelDat(bytes);
	levelDatTime = file.lastModified();
}

void World::readFromZip(const QFileInfo &file)
{
	QuaZip zip(file.absoluteFilePath());
	is_valid = zip.open(QuaZip::mdUnzip);
	if (!is_valid)
	{
		return;
	}
	auto location = MMCZip::findFileInZip(&zip, "level.dat");
	is_valid = !location.isEmpty();
	if (!is_valid)
	{
		return;
	}
	m_containerOffsetPath = location;
	QuaZipFile zippedFile(&zip);
	// read the install profile
	is_valid = zip.setCurrentFile(location + "level.dat");
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
	loadFromLevelDat(zippedFile.readAll());
	zippedFile.close();
}

bool World::install(const QString &to, const QString &name)
{
	auto finalPath = FS::PathCombine(to, FS::DirNameFromString(m_actualName, to));
	if(!FS::ensureFolderPathExists(finalPath))
	{
		return false;
	}
	bool ok = false;
	if(m_containerFile.isFile())
	{
		QuaZip zip(m_containerFile.absoluteFilePath());
		if (!zip.open(QuaZip::mdUnzip))
		{
			return false;
		}
		ok = !MMCZip::extractSubDir(&zip, m_containerOffsetPath, finalPath).isEmpty();
	}
	else if(m_containerFile.isDir())
	{
		QString from = m_containerFile.filePath();
		ok = FS::copy(from, finalPath)();
	}

	if(ok && !name.isEmpty() && m_actualName != name)
	{
		World newWorld(finalPath);
		if(newWorld.isValid())
		{
			newWorld.rename(name);
		}
	}
	return ok;
}

bool World::rename(const QString &newName)
{
	if(m_containerFile.isFile())
	{
		return false;
	}

	auto data = getLevelDatDataFromFS(m_containerFile);
	if(data.isEmpty())
	{
		return false;
	}

	auto worldData = parseLevelDat(data);
	if(!worldData)
	{
		return false;
	}
	auto &val = worldData->at("Data");
	if(val.get_type() != nbt::tag_type::Compound)
	{
		return false;
	}
	auto &dataCompound = val.as<nbt::tag_compound>();
	dataCompound.put("LevelName", nbt::value_initializer(newName.toUtf8().data()));
	data = serializeLevelDat(worldData.get());

	putLevelDatDataToFS(m_containerFile, data);

	m_actualName = newName;

	QDir parentDir(m_containerFile.absoluteFilePath());
	parentDir.cdUp();
	QFile container(m_containerFile.absoluteFilePath());
	auto dirName = FS::DirNameFromString(m_actualName, parentDir.absolutePath());
	container.rename(parentDir.absoluteFilePath(dirName));

	return true;
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

void World::loadFromLevelDat(QByteArray data)
{
	try
	{
		auto levelData = parseLevelDat(data);
		if(!levelData)
		{
			is_valid = false;
			return;
		}

		auto &val = levelData->at("Data");
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
	bool success = FS::copy(with.m_containerFile.filePath(), m_containerFile.path())();
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
