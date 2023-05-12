// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <QDir>
#include <QString>
#include <QDebug>
#include <QSaveFile>
#include <QDirIterator>
#include "World.h"

#include "GZip.h"
#include <MMCZip.h>
#include <FileSystem.h>
#include <sstream>
#include <io/stream_reader.h>
#include <tag_string.h>
#include <tag_primitive.h>
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#include <quazip/quazipdir.h>

#include <QCoreApplication>

#include <optional>

#include "FileSystem.h"

using std::optional;
using std::nullopt;

GameType::GameType(std::optional<int> original):
    original(original)
{
    if(!original) {
        return;
    }
    switch(*original) {
        case 0:
            type = GameType::Survival;
            break;
        case 1:
            type = GameType::Creative;
            break;
        case 2:
            type = GameType::Adventure;
            break;
        case 3:
            type = GameType::Spectator;
            break;
        default:
            break;
    }
}

QString GameType::toTranslatedString() const
{
    switch (type)
    {
        case GameType::Survival:
            return QCoreApplication::translate("GameType", "Survival");
        case GameType::Creative:
            return QCoreApplication::translate("GameType", "Creative");
        case GameType::Adventure:
            return QCoreApplication::translate("GameType", "Adventure");
        case GameType::Spectator:
            return QCoreApplication::translate("GameType", "Spectator");
        default:
            break;
    }
    if(original) {
        return QCoreApplication::translate("GameType", "Unknown (%1)").arg(*original);
    }
    return QCoreApplication::translate("GameType", "Undefined");
}

QString GameType::toLogString() const
{
    switch (type)
    {
        case GameType::Survival:
            return "Survival";
        case GameType::Creative:
            return "Creative";
        case GameType::Adventure:
            return "Adventure";
        case GameType::Spectator:
            return "Spectator";
        default:
            break;
    }
    if(original) {
        return QString("Unknown (%1)").arg(*original);
    }
    return "Undefined";
}

std::unique_ptr <nbt::tag_compound> parseLevelDat(QByteArray data)
{
    QByteArray output;
    if(!GZip::unzip(data, output))
    {
        return nullptr;
    }
    std::istringstream foo(std::string(output.constData(), output.size()));
    try {
        auto pair = nbt::io::read_compound(foo);

        if(pair.first != "")
            return nullptr;

        if(pair.second == nullptr)
            return nullptr;

        return std::move(pair.second);
    }
    catch (const nbt::io::input_error &e)
    {
        qWarning() << "Unable to parse level.dat:" << e.what();
        return nullptr;
    }
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

int64_t calculateWorldSize(const QFileInfo &file)
{
    if (file.isFile() && file.suffix() == "zip")
    {
        return file.size();
    }
    else if(file.isDir())
    {
        QDirIterator it(file.absoluteFilePath(), QDir::Files, QDirIterator::Subdirectories);
        int64_t total = 0;
        while (it.hasNext())
        {
            total += it.fileInfo().size();
            it.next();
        }
        return total;
    }
    return -1;
}

World::World(const QFileInfo &file)
{
    repath(file);
}

void World::repath(const QFileInfo &file)
{
    m_containerFile = file;
    m_folderName = file.fileName();
    m_size = calculateWorldSize(file);
    if(file.isFile() && file.suffix() == "zip")
    {
        m_iconFile = QString();
        readFromZip(file);
    }
    else if(file.isDir())
    {
        QFileInfo assumedIconPath(file.absoluteFilePath() + "/icon.png");
        if(assumedIconPath.exists()) {
            m_iconFile = assumedIconPath.absoluteFilePath();
        }
        readFromFS(file);
    }
}

bool World::resetIcon()
{
    if(m_iconFile.isNull()) {
        return false;
    }
    if(QFile(m_iconFile).remove()) {
        m_iconFile = QString();
        return true;
    }
    return false;
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
    auto location = MMCZip::findFolderOfFileInZip(&zip, "level.dat");
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
        ok = !MMCZip::extractSubDir(&zip, m_containerOffsetPath, finalPath);
    }
    else if(m_containerFile.isDir())
    {
        QString from = m_containerFile.filePath();
        ok = FS::copy(from, finalPath)();
    }

    if(ok && !name.isEmpty() && m_actualName != name)
    {
        QFileInfo finalPathInfo(finalPath);
        World newWorld(finalPathInfo);
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

namespace {

optional<QString> read_string (nbt::value& parent, const char * name)
{
    try
    {
        auto &namedValue = parent.at(name);
        if(namedValue.get_type() != nbt::tag_type::String)
        {
            return nullopt;
        }
        auto & tag_str = namedValue.as<nbt::tag_string>();
        return QString::fromStdString(tag_str.get());
    }
    catch (const std::out_of_range &e)
    {
        // fallback for old world formats
        qWarning() << "String NBT tag" << name << "could not be found.";
        return nullopt;
    }
    catch (const std::bad_cast &e)
    {
        // type mismatch
        qWarning() << "NBT tag" << name << "could not be converted to string.";
        return nullopt;
    }
}

optional<int64_t> read_long (nbt::value& parent, const char * name)
{
    try
    {
        auto &namedValue = parent.at(name);
        if(namedValue.get_type() != nbt::tag_type::Long)
        {
            return nullopt;
        }
        auto & tag_str = namedValue.as<nbt::tag_long>();
        return tag_str.get();
    }
    catch (const std::out_of_range &e)
    {
        // fallback for old world formats
        qWarning() << "Long NBT tag" << name << "could not be found.";
        return nullopt;
    }
    catch (const std::bad_cast &e)
    {
        // type mismatch
        qWarning() << "NBT tag" << name << "could not be converted to long.";
        return nullopt;
    }
}

optional<int> read_int (nbt::value& parent, const char * name)
{
    try
    {
        auto &namedValue = parent.at(name);
        if(namedValue.get_type() != nbt::tag_type::Int)
        {
            return nullopt;
        }
        auto & tag_str = namedValue.as<nbt::tag_int>();
        return tag_str.get();
    }
    catch (const std::out_of_range &e)
    {
        // fallback for old world formats
        qWarning() << "Int NBT tag" << name << "could not be found.";
        return nullopt;
    }
    catch (const std::bad_cast &e)
    {
        // type mismatch
        qWarning() << "NBT tag" << name << "could not be converted to int.";
        return nullopt;
    }
}

GameType read_gametype(nbt::value& parent, const char * name) {
    return GameType(read_int(parent, name));
}

}

void World::loadFromLevelDat(QByteArray data)
{
    auto levelData = parseLevelDat(data);
    if(!levelData)
    {
        is_valid = false;
        return;
    }

    nbt::value * valPtr = nullptr;
    try {
        valPtr = &levelData->at("Data");
    }
    catch (const std::out_of_range &e) {
        qWarning() << "Unable to read NBT tags from " << m_folderName << ":" << e.what();
        is_valid = false;
        return;
    }
    nbt::value &val = *valPtr;

    is_valid = val.get_type() == nbt::tag_type::Compound;
    if(!is_valid)
        return;

    auto name = read_string(val, "LevelName");
    m_actualName = name ? *name : m_folderName;

    auto timestamp = read_long(val, "LastPlayed");
    m_lastPlayed = timestamp ? QDateTime::fromMSecsSinceEpoch(*timestamp) : levelDatTime;

    m_gameType = read_gametype(val, "GameType");

    optional<int64_t> randomSeed;
    try {
        auto &WorldGen_val = val.at("WorldGenSettings");
        randomSeed = read_long(WorldGen_val, "seed");
    }
    catch (const std::out_of_range &) {}
    if(!randomSeed) {
        randomSeed = read_long(val, "RandomSeed");
    }
    m_randomSeed = randomSeed ? *randomSeed : 0;

    qDebug() << "World Name:" << m_actualName;
    qDebug() << "Last Played:" << m_lastPlayed.toString();
    if(randomSeed) {
        qDebug() << "Seed:" << *randomSeed;
    }
    qDebug() << "Size:" << m_size;
    qDebug() << "GameType:" << m_gameType.toLogString();
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

    if (FS::trash(m_containerFile.filePath()))
        return true;

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

bool World::isSymLinkUnder(const QString& instPath) const
{
    if (isSymLink())
        return true;

    auto instDir = QDir(instPath);

    auto relAbsPath = instDir.relativeFilePath(m_containerFile.absoluteFilePath());
    auto relCanonPath = instDir.relativeFilePath(m_containerFile.canonicalFilePath());

    return relAbsPath != relCanonPath;
}

bool World::isMoreThanOneHardLink() const
{
    if (m_containerFile.isDir())
    {
        return FS::hardLinkCount(QDir(m_containerFile.absoluteFilePath()).filePath("level.dat")) > 1;
    }
    return FS::hardLinkCount(m_containerFile.absoluteFilePath()) > 1;
}
