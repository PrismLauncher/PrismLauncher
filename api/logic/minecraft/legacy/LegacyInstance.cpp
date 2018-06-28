/* Copyright 2013-2018 MultiMC Contributors
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

#include <QFileInfo>
#include <minecraft/launch/LauncherPartLaunch.h>
#include <QDir>
#include <settings/Setting.h>

#include "LegacyInstance.h"

#include "minecraft/legacy/LegacyModList.h"
#include "minecraft/ModList.h"
#include "minecraft/WorldList.h"
#include <MMCZip.h>
#include <FileSystem.h>

LegacyInstance::LegacyInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
	: BaseInstance(globalSettings, settings, rootDir)
{
	settings->registerSetting("NeedsRebuild", true);
	settings->registerSetting("ShouldUpdate", false);
	settings->registerSetting("JarVersion", QString());
	settings->registerSetting("IntendedJarVersion", QString());
	/*
	 * custom base jar has no default. it is determined in code... see the accessor methods for
	 *it
	 *
	 * for instances that DO NOT have the CustomBaseJar setting (legacy instances),
	 * [.]minecraft/bin/mcbackup.jar is the default base jar
	 */
	settings->registerSetting("UseCustomBaseJar", true);
	settings->registerSetting("CustomBaseJar", "");
}

QString LegacyInstance::mainJarToPreserve() const
{
	bool customJar = m_settings->get("UseCustomBaseJar").toBool();
	if(customJar)
	{
		auto base = baseJar();
		if(QFile::exists(base))
		{
			return base;
		}
	}
	auto runnable = runnableJar();
	if(QFile::exists(runnable))
	{
		return runnable;
	}
	return QString();
}


QString LegacyInstance::baseJar() const
{
	bool customJar = m_settings->get("UseCustomBaseJar").toBool();
	if (customJar)
	{
		return customBaseJar();
	}
	else
		return defaultBaseJar();
}

QString LegacyInstance::customBaseJar() const
{
	QString value = m_settings->get("CustomBaseJar").toString();
	if (value.isNull() || value.isEmpty())
	{
		return defaultCustomBaseJar();
	}
	return value;
}

bool LegacyInstance::shouldUseCustomBaseJar() const
{
	return m_settings->get("UseCustomBaseJar").toBool();
}


shared_qobject_ptr<Task> LegacyInstance::createUpdateTask(Net::Mode)
{
	return nullptr;
}

std::shared_ptr<LegacyModList> LegacyInstance::jarModList() const
{
	if (!jar_mod_list)
	{
		auto list = new LegacyModList(jarModsDir(), modListFile());
		jar_mod_list.reset(list);
	}
	jar_mod_list->update();
	return jar_mod_list;
}

QList<Mod> LegacyInstance::getJarMods() const
{
	return jarModList()->allMods();
}

QString LegacyInstance::minecraftRoot() const
{
	QFileInfo mcDir(FS::PathCombine(instanceRoot(), "minecraft"));
	QFileInfo dotMCDir(FS::PathCombine(instanceRoot(), ".minecraft"));

	if (mcDir.exists() && !dotMCDir.exists())
		return mcDir.filePath();
	else
		return dotMCDir.filePath();
}

QString LegacyInstance::binRoot() const
{
	return FS::PathCombine(minecraftRoot(), "bin");
}

QString LegacyInstance::jarModsDir() const
{
	return FS::PathCombine(instanceRoot(), "instMods");
}

QString LegacyInstance::libDir() const
{
	return FS::PathCombine(minecraftRoot(), "lib");
}

QString LegacyInstance::savesDir() const
{
	return FS::PathCombine(minecraftRoot(), "saves");
}

QString LegacyInstance::loaderModsDir() const
{
	return FS::PathCombine(minecraftRoot(), "mods");
}

QString LegacyInstance::coreModsDir() const
{
	return FS::PathCombine(minecraftRoot(), "coremods");
}

QString LegacyInstance::resourceDir() const
{
	return FS::PathCombine(minecraftRoot(), "resources");
}
QString LegacyInstance::texturePacksDir() const
{
	return FS::PathCombine(minecraftRoot(), "texturepacks");
}

QString LegacyInstance::runnableJar() const
{
	return FS::PathCombine(binRoot(), "minecraft.jar");
}

QString LegacyInstance::modListFile() const
{
	return FS::PathCombine(instanceRoot(), "modlist");
}

QString LegacyInstance::instanceConfigFolder() const
{
	return FS::PathCombine(minecraftRoot(), "config");
}

bool LegacyInstance::shouldRebuild() const
{
	return m_settings->get("NeedsRebuild").toBool();
}

QString LegacyInstance::currentVersionId() const
{
	return m_settings->get("JarVersion").toString();
}

QString LegacyInstance::intendedVersionId() const
{
	return m_settings->get("IntendedJarVersion").toString();
}

bool LegacyInstance::shouldUpdate() const
{
	QVariant var = settings()->get("ShouldUpdate");
	if (!var.isValid() || var.toBool() == false)
	{
		return intendedVersionId() != currentVersionId();
	}
	return true;
}

QString LegacyInstance::defaultBaseJar() const
{
	return "versions/" + intendedVersionId() + "/" + intendedVersionId() + ".jar";
}

QString LegacyInstance::defaultCustomBaseJar() const
{
	return FS::PathCombine(binRoot(), "mcbackup.jar");
}

std::shared_ptr<WorldList> LegacyInstance::worldList() const
{
	if (!m_world_list)
	{
		m_world_list.reset(new WorldList(savesDir()));
	}
	return m_world_list;
}

QString LegacyInstance::typeName() const
{
	return tr("Legacy");
}

QString LegacyInstance::getStatusbarDescription()
{
	return tr("Instance from previous versions.");
}

QStringList LegacyInstance::verboseDescription(AuthSessionPtr session)
{
	QStringList out;

	auto alltraits = traits();
	if(alltraits.size())
	{
		out << "Traits:";
		for (auto trait : alltraits)
		{
			out << "  " + trait;
		}
		out << "";
	}

	QString windowParams;
	if (settings()->get("LaunchMaximized").toBool())
	{
		out << "Window size: max (if available)";
	}
	else
	{
		auto width = settings()->get("MinecraftWinWidth").toInt();
		auto height = settings()->get("MinecraftWinHeight").toInt();
		out << "Window size: " + QString::number(width) + " x " + QString::number(height);
	}
	out << "";
	return out;
}
