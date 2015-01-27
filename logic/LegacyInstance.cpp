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

#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <logic/settings/Setting.h>
#include <pathutils.h>
#include <cmdutils.h>

#include "MultiMC.h"

#include "LegacyInstance.h"

#include "logic/LegacyUpdate.h"
#include "logic/icons/IconList.h"
#include "logic/minecraft/MinecraftProcess.h"
#include "gui/pages/LegacyUpgradePage.h"
#include "gui/pages/ModFolderPage.h"
#include "gui/pages/LegacyJarModPage.h"
#include <gui/pages/TexturePackPage.h>
#include <gui/pages/InstanceSettingsPage.h>
#include <gui/pages/NotesPage.h>
#include <gui/pages/ScreenshotsPage.h>

LegacyInstance::LegacyInstance(const QString &rootDir, SettingsObject *settings, QObject *parent)
	: MinecraftInstance(rootDir, settings, parent)
{
	settings->registerSetting("NeedsRebuild", true);
	settings->registerSetting("ShouldUpdate", false);
	settings->registerSetting("JarVersion", "Unknown");
	settings->registerSetting("LwjglVersion", "2.9.0");
	settings->registerSetting("IntendedJarVersion", "");
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

QList<BasePage *> LegacyInstance::getPages()
{
	QList<BasePage *> values;
	// FIXME: actually implement the legacy instance upgrade, then enable this.
	//values.append(new LegacyUpgradePage(this));
	values.append(new LegacyJarModPage(this));
	values.append(new ModFolderPage(this, loaderModList(), "mods", "loadermods", tr("Loader mods"),
									"Loader-mods"));
	values.append(new ModFolderPage(this, coreModList(), "coremods", "coremods", tr("Core mods"),
									"Loader-mods"));
	values.append(new TexturePackPage(this));
	values.append(new NotesPage(this));
	values.append(new ScreenshotsPage(PathCombine(minecraftRoot(), "screenshots")));
	values.append(new InstanceSettingsPage(this));
	return values;
}

QString LegacyInstance::dialogTitle()
{
	return tr("Edit Instance (%1)").arg(name());
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

void LegacyInstance::setCustomBaseJar(QString val)
{
	if (val.isNull() || val.isEmpty() || val == defaultCustomBaseJar())
		m_settings->reset("CustomBaseJar");
	else
		m_settings->set("CustomBaseJar", val);
}

void LegacyInstance::setShouldUseCustomBaseJar(bool val)
{
	m_settings->set("UseCustomBaseJar", val);
}

bool LegacyInstance::shouldUseCustomBaseJar() const
{
	return m_settings->get("UseCustomBaseJar").toBool();
}


std::shared_ptr<Task> LegacyInstance::doUpdate()
{
	// make sure the jar mods list is initialized by asking for it.
	auto list = jarModList();
	// create an update task
	return std::shared_ptr<Task>(new LegacyUpdate(this, this));
}

BaseProcess *LegacyInstance::prepareForLaunch(AuthSessionPtr account)
{
	QString launchScript;
	QIcon icon = MMC->icons()->getIcon(iconKey());
	auto pixmap = icon.pixmap(128, 128);
	pixmap.save(PathCombine(minecraftRoot(), "icon.png"), "PNG");

	// create the launch script
	{
		// window size
		QString windowParams;
		if (settings().get("LaunchMaximized").toBool())
			windowParams = "max";
		else
			windowParams = QString("%1x%2")
							   .arg(settings().get("MinecraftWinWidth").toInt())
							   .arg(settings().get("MinecraftWinHeight").toInt());

		QString lwjgl = QDir(MMC->settings()->get("LWJGLDir").toString() + "/" + lwjglVersion())
							.absolutePath();
		launchScript += "userName " + account->player_name + "\n";
		launchScript += "sessionId " + account->session + "\n";
		launchScript += "windowTitle " + windowTitle() + "\n";
		launchScript += "windowParams " + windowParams + "\n";
		launchScript += "lwjgl " + lwjgl + "\n";
		launchScript += "launcher legacy\n";
	}
	auto process = MinecraftProcess::create(std::dynamic_pointer_cast<MinecraftInstance>(getSharedPtr()));
	process->setLaunchScript(launchScript);
	process->setWorkdir(minecraftRoot());
	process->setLogin(account);
	return process;
}

void LegacyInstance::cleanupAfterRun()
{
	// FIXME: delete the launcher and icons and whatnot.
}

std::shared_ptr<ModList> LegacyInstance::coreModList() const
{
	if (!core_mod_list)
	{
		core_mod_list.reset(new ModList(coreModsDir()));
	}
	core_mod_list->update();
	return core_mod_list;
}

std::shared_ptr<ModList> LegacyInstance::jarModList() const
{
	if (!jar_mod_list)
	{
		auto list = new ModList(jarModsDir(), modListFile());
		connect(list, SIGNAL(changed()), SLOT(jarModsChanged()));
		jar_mod_list.reset(list);
	}
	jar_mod_list->update();
	return jar_mod_list;
}

QList<Mod> LegacyInstance::getJarMods() const
{
	return jarModList()->allMods();
}

void LegacyInstance::jarModsChanged()
{
	QLOG_INFO() << "Jar mods of instance " << name() << " have changed. Jar will be rebuilt.";
	setShouldRebuild(true);
}

std::shared_ptr<ModList> LegacyInstance::loaderModList() const
{
	if (!loader_mod_list)
	{
		loader_mod_list.reset(new ModList(loaderModsDir()));
	}
	loader_mod_list->update();
	return loader_mod_list;
}

std::shared_ptr<ModList> LegacyInstance::texturePackList() const
{
	if (!texture_pack_list)
	{
		texture_pack_list.reset(new ModList(texturePacksDir()));
	}
	texture_pack_list->update();
	return texture_pack_list;
}

QString LegacyInstance::jarModsDir() const
{
	return PathCombine(instanceRoot(), "instMods");
}

QString LegacyInstance::binDir() const
{
	return PathCombine(minecraftRoot(), "bin");
}

QString LegacyInstance::libDir() const
{
	return PathCombine(minecraftRoot(), "lib");
}

QString LegacyInstance::savesDir() const
{
	return PathCombine(minecraftRoot(), "saves");
}

QString LegacyInstance::loaderModsDir() const
{
	return PathCombine(minecraftRoot(), "mods");
}

QString LegacyInstance::coreModsDir() const
{
	return PathCombine(minecraftRoot(), "coremods");
}

QString LegacyInstance::resourceDir() const
{
	return PathCombine(minecraftRoot(), "resources");
}
QString LegacyInstance::texturePacksDir() const
{
	return PathCombine(minecraftRoot(), "texturepacks");
}

QString LegacyInstance::runnableJar() const
{
	return PathCombine(binDir(), "minecraft.jar");
}

QString LegacyInstance::modListFile() const
{
	return PathCombine(instanceRoot(), "modlist");
}

QString LegacyInstance::instanceConfigFolder() const
{
	return PathCombine(minecraftRoot(), "config");
}

bool LegacyInstance::shouldRebuild() const
{
	return m_settings->get("NeedsRebuild").toBool();
}

void LegacyInstance::setShouldRebuild(bool val)
{
	m_settings->set("NeedsRebuild", val);
}

QString LegacyInstance::currentVersionId() const
{
	return m_settings->get("JarVersion").toString();
}

QString LegacyInstance::lwjglVersion() const
{
	return m_settings->get("LwjglVersion").toString();
}

void LegacyInstance::setLWJGLVersion(QString val)
{
	m_settings->set("LwjglVersion", val);
}

QString LegacyInstance::intendedVersionId() const
{
	return m_settings->get("IntendedJarVersion").toString();
}

bool LegacyInstance::setIntendedVersionId(QString version)
{
	settings().set("IntendedJarVersion", version);
	setShouldUpdate(true);
	return true;
}

bool LegacyInstance::shouldUpdate() const
{
	QVariant var = settings().get("ShouldUpdate");
	if (!var.isValid() || var.toBool() == false)
	{
		return intendedVersionId() != currentVersionId();
	}
	return true;
}

void LegacyInstance::setShouldUpdate(bool val)
{
	settings().set("ShouldUpdate", val);
}

QString LegacyInstance::defaultBaseJar() const
{
	return "versions/" + intendedVersionId() + "/" + intendedVersionId() + ".jar";
}

QString LegacyInstance::defaultCustomBaseJar() const
{
	return PathCombine(binDir(), "mcbackup.jar");
}

QString LegacyInstance::getStatusbarDescription()
{
	if (flags() & VersionBrokenFlag)
	{
		return tr("Legacy : %1 (broken)").arg(intendedVersionId());
	}
	return tr("Legacy : %1").arg(intendedVersionId());
}
