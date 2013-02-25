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

#include "include/instance.h"

#include <QFileInfo>

#include "settingsobject.h"

#include "pathutils.h"

Instance::Instance(const QString &rootDir, QObject *parent) :
	QObject(parent)
{
	m_rootDir = rootDir;
	config.loadFile(PathCombine(rootDir, "instance.cfg"));
}

QString Instance::id() const
{
	return QFileInfo(rootDir()).fileName();
}

QString Instance::rootDir() const
{
	return m_rootDir;
}

InstanceList *Instance::instList() const
{
	if (parent()->inherits("InstanceList"))
		return (InstanceList *)parent();
	else
		return NULL;
}

QString Instance::minecraftDir() const
{
	QFileInfo mcDir(PathCombine(rootDir(), "minecraft"));
	QFileInfo dotMCDir(PathCombine(rootDir(), ".minecraft"));
	
	if (dotMCDir.exists() && !mcDir.exists())
        return dotMCDir.filePath();
	else
        return mcDir.filePath();
}

QString Instance::binDir() const
{
	return PathCombine(minecraftDir(), "bin");
}

QString Instance::savesDir() const
{
	return PathCombine(minecraftDir(), "saves");
}

QString Instance::mlModsDir() const
{
	return PathCombine(minecraftDir(), "mods");
}

QString Instance::coreModsDir() const
{
	return PathCombine(minecraftDir(), "coremods");
}

QString Instance::resourceDir() const
{
	return PathCombine(minecraftDir(), "resources");
}

QString Instance::screenshotsDir() const
{
	return PathCombine(minecraftDir(), "screenshots");
}

QString Instance::texturePacksDir() const
{
	return PathCombine(minecraftDir(), "texturepacks");
}

QString Instance::mcJar() const
{
	return PathCombine(binDir(), "minecraft.jar");
}

QVariant Instance::getField(const QString &name, QVariant defVal) const
{
	return config.get(name, defVal);
}

void Instance::setField(const QString &name, QVariant val)
{
	config.set(name, val);
}

SettingsObject &Instance::settings()
{
	return *globalSettings;
}
