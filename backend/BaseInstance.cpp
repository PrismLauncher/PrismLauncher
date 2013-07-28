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

#include "BaseInstance.h"

#include <QFileInfo>

#include "inisettingsobject.h"
#include "setting.h"
#include "overridesetting.h"

#include "pathutils.h"
#include <lists/MinecraftVersionList.h>

BaseInstance::BaseInstance(const QString &rootDir, QObject *parent) :
	QObject(parent)
{
	m_rootDir = rootDir;
	m_settings = new INISettingsObject(PathCombine(rootDir, "instance.cfg"), this);
	
	settings().registerSetting(new Setting("name", "Unnamed Instance"));
	settings().registerSetting(new Setting("iconKey", "default"));
	settings().registerSetting(new Setting("notes", ""));
	settings().registerSetting(new Setting("NeedsRebuild", true));
	settings().registerSetting(new Setting("ShouldUpdate", false));
	settings().registerSetting(new Setting("JarVersion", "Unknown"));
	settings().registerSetting(new Setting("LwjglVersion", "2.9.0"));
	settings().registerSetting(new Setting("IntendedJarVersion", ""));
	settings().registerSetting(new Setting("lastLaunchTime", 0));
	
	// Java Settings
	settings().registerSetting(new OverrideSetting("JavaPath", globalSettings->getSetting("JavaPath")));
	settings().registerSetting(new OverrideSetting("JvmArgs", globalSettings->getSetting("JvmArgs")));
	
	// Custom Commands
	settings().registerSetting(new OverrideSetting("PreLaunchCommand", 
												   globalSettings->getSetting("PreLaunchCommand")));
	settings().registerSetting(new OverrideSetting("PostExitCommand", 
												   globalSettings->getSetting("PostExitCommand")));
	
	// Window Size
	settings().registerSetting(new OverrideSetting("LaunchCompatMode", globalSettings->getSetting("LaunchCompatMode")));
	settings().registerSetting(new OverrideSetting("LaunchMaximized", globalSettings->getSetting("LaunchMaximized")));
	settings().registerSetting(new OverrideSetting("MinecraftWinWidth", globalSettings->getSetting("MinecraftWinWidth")));
	settings().registerSetting(new OverrideSetting("MinecraftWinHeight", globalSettings->getSetting("MinecraftWinHeight")));
	
	// Memory
	settings().registerSetting(new OverrideSetting("MinMemAlloc", globalSettings->getSetting("MinMemAlloc")));
	settings().registerSetting(new OverrideSetting("MaxMemAlloc", globalSettings->getSetting("MaxMemAlloc")));
	
	// Auto login
	settings().registerSetting(new OverrideSetting("AutoLogin", globalSettings->getSetting("AutoLogin")));
	
	// Console
	settings().registerSetting(new OverrideSetting("ShowConsole", globalSettings->getSetting("ShowConsole")));
	settings().registerSetting(new OverrideSetting("AutoCloseConsole", globalSettings->getSetting("AutoCloseConsole")));
	
	// Overrides
	settings().registerSetting(new Setting("OverrideConsole", false));
	settings().registerSetting(new Setting("OverrideWindow", false));
	settings().registerSetting(new Setting("OverrideLogin", false));
	settings().registerSetting(new Setting("OverrideMemory", false));
	settings().registerSetting(new Setting("OverrideJava", false));
	settings().registerSetting(new Setting("OverrideCommands", false));
}

QString BaseInstance::id() const
{
	return QFileInfo(rootDir()).fileName();
}

QString BaseInstance::rootDir() const
{
	return m_rootDir;
}

InstanceList *BaseInstance::instList() const
{
	if (parent()->inherits("InstanceList"))
		return (InstanceList *)parent();
	else
		return NULL;
}

QString BaseInstance::minecraftDir() const
{
	QFileInfo mcDir(PathCombine(rootDir(), "minecraft"));
	QFileInfo dotMCDir(PathCombine(rootDir(), ".minecraft"));
	
	if (dotMCDir.exists() && !mcDir.exists())
        return dotMCDir.filePath();
	else
		return mcDir.filePath();
}

QString BaseInstance::instModsDir() const
{
	return PathCombine(rootDir(), "instMods");
}

QString BaseInstance::binDir() const
{
	return PathCombine(minecraftDir(), "bin");
}

QString BaseInstance::savesDir() const
{
	return PathCombine(minecraftDir(), "saves");
}

QString BaseInstance::mlModsDir() const
{
	return PathCombine(minecraftDir(), "mods");
}

QString BaseInstance::coreModsDir() const
{
	return PathCombine(minecraftDir(), "coremods");
}

QString BaseInstance::resourceDir() const
{
	return PathCombine(minecraftDir(), "resources");
}

QString BaseInstance::screenshotsDir() const
{
	return PathCombine(minecraftDir(), "screenshots");
}

QString BaseInstance::texturePacksDir() const
{
	return PathCombine(minecraftDir(), "texturepacks");
}

QString BaseInstance::mcJar() const
{
	return PathCombine(binDir(), "minecraft.jar");
}

QString BaseInstance::mcBackup() const
{
	return PathCombine(binDir(), "mcbackup.jar");
}

QString BaseInstance::modListFile() const
{
	return PathCombine(rootDir(), "modlist");
}

InstVersionList *BaseInstance::versionList() const
{
	return &MinecraftVersionList::getMainList();
}

bool BaseInstance::shouldUpdateCurrentVersion() const
{
	QFileInfo jar(mcJar());
	return jar.lastModified().toUTC().toMSecsSinceEpoch() != lastCurrentVersionUpdate();
}

void BaseInstance::updateCurrentVersion(bool keepCurrent)
{
	QFileInfo jar(mcJar());
	
	if(!jar.exists())
	{
		setLastCurrentVersionUpdate(0);
		setCurrentVersion("Unknown");
		return;
	}
	
	qint64 time = jar.lastModified().toUTC().toMSecsSinceEpoch();
	
	setLastCurrentVersionUpdate(time);
	if (!keepCurrent)
	{
		// TODO: Implement GetMinecraftJarVersion function.
		QString newVersion = "Unknown";//javautils::GetMinecraftJarVersion(jar.absoluteFilePath());
		setCurrentVersion(newVersion);
	}
}

SettingsObject &BaseInstance::settings() const
{
	return *m_settings;
}
