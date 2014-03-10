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

#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <setting.h>
#include <pathutils.h>
#include <cmdutils.h>

#include "MultiMC.h"

#include "LegacyInstance.h"
#include "LegacyInstance_p.h"

#include "logic/MinecraftProcess.h"
#include "logic/LegacyUpdate.h"
#include "logic/icons/IconList.h"

#include "gui/dialogs/LegacyModEditDialog.h"

LegacyInstance::LegacyInstance(const QString &rootDir, SettingsObject *settings,
							   QObject *parent)
	: BaseInstance(new LegacyInstancePrivate(), rootDir, settings, parent)
{
	settings->registerSetting("NeedsRebuild", true);
	settings->registerSetting("ShouldUpdate", false);
	settings->registerSetting("JarVersion", "Unknown");
	settings->registerSetting("LwjglVersion", "2.9.0");
	settings->registerSetting("IntendedJarVersion", "");
}

std::shared_ptr<Task> LegacyInstance::doUpdate()
{
	// make sure the jar mods list is initialized by asking for it.
	auto list = jarModList();
	// create an update task
	return std::shared_ptr<Task>(new LegacyUpdate(this, this));
}

MinecraftProcess *LegacyInstance::prepareForLaunch(AuthSessionPtr account)
{
	MinecraftProcess *proc = new MinecraftProcess(this);

	QIcon icon = MMC->icons()->getIcon(iconKey());
	auto pixmap = icon.pixmap(128, 128);
	pixmap.save(PathCombine(minecraftRoot(), "icon.png"), "PNG");

	// create the launch script
	QString launchScript;
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
	proc->setLaunchScript(launchScript);

	// set the process work path
	proc->setWorkdir(minecraftRoot());

	return proc;
}

void LegacyInstance::cleanupAfterRun()
{
	// FIXME: delete the launcher and icons and whatnot.
}

std::shared_ptr<ModList> LegacyInstance::coreModList()
{
	I_D(LegacyInstance);
	if (!d->core_mod_list)
	{
		d->core_mod_list.reset(new ModList(coreModsDir()));
	}
	d->core_mod_list->update();
	return d->core_mod_list;
}

std::shared_ptr<ModList> LegacyInstance::jarModList()
{
	I_D(LegacyInstance);
	if (!d->jar_mod_list)
	{
		auto list = new ModList(jarModsDir(), modListFile());
		connect(list, SIGNAL(changed()), SLOT(jarModsChanged()));
		d->jar_mod_list.reset(list);
	}
	d->jar_mod_list->update();
	return d->jar_mod_list;
}

void LegacyInstance::jarModsChanged()
{
	QLOG_INFO() << "Jar mods of instance " << name() << " have changed. Jar will be rebuilt.";
	setShouldRebuild(true);
}

std::shared_ptr<ModList> LegacyInstance::loaderModList()
{
	I_D(LegacyInstance);
	if (!d->loader_mod_list)
	{
		d->loader_mod_list.reset(new ModList(loaderModsDir()));
	}
	d->loader_mod_list->update();
	return d->loader_mod_list;
}

std::shared_ptr<ModList> LegacyInstance::texturePackList()
{
	I_D(LegacyInstance);
	if (!d->texture_pack_list)
	{
		d->texture_pack_list.reset(new ModList(texturePacksDir()));
	}
	d->texture_pack_list->update();
	return d->texture_pack_list;
}

QDialog *LegacyInstance::createModEditDialog(QWidget *parent)
{
	return new LegacyModEditDialog(this, parent);
}

QString LegacyInstance::jarModsDir() const
{
	return PathCombine(instanceRoot(), "instMods");
}

QString LegacyInstance::binDir() const
{
	return PathCombine(minecraftRoot(), "bin");
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
	I_D(LegacyInstance);
	return d->m_settings->get("NeedsRebuild").toBool();
}

void LegacyInstance::setShouldRebuild(bool val)
{
	I_D(LegacyInstance);
	d->m_settings->set("NeedsRebuild", val);
}

QString LegacyInstance::currentVersionId() const
{
	I_D(LegacyInstance);
	return d->m_settings->get("JarVersion").toString();
}

QString LegacyInstance::lwjglVersion() const
{
	I_D(LegacyInstance);
	return d->m_settings->get("LwjglVersion").toString();
}

void LegacyInstance::setLWJGLVersion(QString val)
{
	I_D(LegacyInstance);
	d->m_settings->set("LwjglVersion", val);
}

QString LegacyInstance::intendedVersionId() const
{
	I_D(LegacyInstance);
	return d->m_settings->get("IntendedJarVersion").toString();
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

bool LegacyInstance::menuActionEnabled(QString action_name) const
{
	if (flags().contains(VersionBrokenFlag))
	{
		return false;
	}
	if (action_name == "actionChangeInstMCVersion")
	{
		return false;
	}
	return true;
}

QString LegacyInstance::getStatusbarDescription()
{
	if (flags().contains(VersionBrokenFlag))
	{
		return "Legacy : " + intendedVersionId() + " (broken)";
	}
	if (shouldUpdate())
		return "Legacy : " + currentVersionId() + " -> " + intendedVersionId();
	else
		return "Legacy : " + currentVersionId();
}
