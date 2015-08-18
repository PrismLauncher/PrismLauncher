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
#include "World.h"
#include <pathutils.h>

World::World(const QFileInfo &file)
{
	repath(file);
}

void World::repath(const QFileInfo &file)
{
	m_file = file;
	m_name = file.fileName();
	is_valid = file.isDir() && QDir(file.filePath()).exists("level.dat");
}

bool World::replace(World &with)
{
	if (!destroy())
		return false;
	bool success = copyPath(with.m_file.filePath(), m_file.path());
	if (success)
	{
		m_name = with.m_name;
		m_file.refresh();
	}
	return success;
}

bool World::destroy()
{
	if(!is_valid) return false;
	if (m_file.isDir())
	{
		QDir d(m_file.filePath());
		if (d.removeRecursively())
		{
			return true;
		}
		return false;
	}
	else
	{
		return false;
	}
	return true;
}

bool World::operator==(const World &other) const
{
	return is_valid == other.is_valid && name() == other.name();
}
bool World::strongCompare(const World &other) const
{
	return is_valid == other.is_valid && name() == other.name();
}
