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
#include <settings/Setting.h>
#include <pathutils.h>
#include <cmdutils.h>

#include "LegacyInstance.h"

#include "minecraft/LegacyUpdate.h"
#include "icons/IconList.h"
#include "launch/LaunchTask.h"
#include <launch/steps/LaunchMinecraft.h>
#include <launch/steps/PostLaunchCommand.h>
#include <launch/steps/ModMinecraftJar.h>
#include <launch/steps/Update.h>
#include <launch/steps/PreLaunchCommand.h>
#include <launch/steps/TextPrint.h>
#include "minecraft/ModList.h"
#include <MMCZip.h>

LegacyInstance::LegacyInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
	: MinecraftInstance(globalSettings, settings, rootDir)
{
	m_lwjglFolderSetting = globalSettings->getSetting("LWJGLDir");
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


std::shared_ptr<Task> LegacyInstance::createUpdateTask()
{
	// make sure the jar mods list is initialized by asking for it.
	auto list = jarModList();
	// create an update task
	return std::shared_ptr<Task>(new LegacyUpdate(this, this));
}

std::shared_ptr<LaunchTask> LegacyInstance::createLaunchTask(AuthSessionPtr session)
{
	QString launchScript;
	QIcon icon = ENV.icons()->getIcon(iconKey());
	auto pixmap = icon.pixmap(128, 128);
	pixmap.save(PathCombine(minecraftRoot(), "icon.png"), "PNG");

	// create the launch script
	{
		// window size
		QString windowParams;
		if (settings()->get("LaunchMaximized").toBool())
			windowParams = "max";
		else
			windowParams = QString("%1x%2")
							   .arg(settings()->get("MinecraftWinWidth").toInt())
							   .arg(settings()->get("MinecraftWinHeight").toInt());

		QString lwjgl = QDir(m_lwjglFolderSetting->get().toString() + "/" + lwjglVersion())
							.absolutePath();
		launchScript += "userName " + session->player_name + "\n";
		launchScript += "sessionId " + session->session + "\n";
		launchScript += "windowTitle " + windowTitle() + "\n";
		launchScript += "windowParams " + windowParams + "\n";
		launchScript += "lwjgl " + lwjgl + "\n";
		launchScript += "launcher legacy\n";
	}
	auto process = LaunchTask::create(std::dynamic_pointer_cast<MinecraftInstance>(getSharedPtr()));
	auto pptr = process.get();

	// print a header
	{
		process->appendStep(std::make_shared<TextPrint>(pptr, "Minecraft folder is:\n" + minecraftRoot() + "\n\n", MessageLevel::MultiMC));
	}
	// run pre-launch command if that's needed
	if(getPreLaunchCommand().size())
	{
		auto step = std::make_shared<PreLaunchCommand>(pptr);
		step->setWorkingDirectory(minecraftRoot());
		process->appendStep(step);
	}
	// if we aren't in offline mode,.
	if(session->status != AuthSession::PlayableOffline)
	{
		process->appendStep(std::make_shared<Update>(pptr));
	}
	// if there are any jar mods
	if(getJarMods().size())
	{
		auto step = std::make_shared<ModMinecraftJar>(pptr);
		process->appendStep(step);
	}
	// actually launch the game
	{
		auto step = std::make_shared<LaunchMinecraft>(pptr);
		step->setWorkingDirectory(minecraftRoot());
		step->setLaunchScript(launchScript);
		process->appendStep(step);
	}
	// run post-exit command if that's needed
	if(getPostExitCommand().size())
	{
		auto step = std::make_shared<PostLaunchCommand>(pptr);
		step->setWorkingDirectory(minecraftRoot());
		process->appendStep(step);
	}
	if (session)
	{
		process->setCensorFilter(createCensorFilterFromSession(session));
	}
	return process;
}

std::shared_ptr<Task> LegacyInstance::createJarModdingTask()
{
	class JarModTask : public Task
	{
	public:
		explicit JarModTask(std::shared_ptr<LegacyInstance> inst) : m_inst(inst), Task(nullptr)
		{
		}
		virtual void executeTask()
		{
			if (!m_inst->shouldRebuild())
			{
				emitSucceeded();
				return;
			}

			// Get the mod list
			auto modList = m_inst->getJarMods();

			QFileInfo runnableJar(m_inst->runnableJar());
			QFileInfo baseJar(m_inst->baseJar());
			bool base_is_custom = m_inst->shouldUseCustomBaseJar();

			// Nothing to do if there are no jar mods to install, no backup and just the mc jar
			if (base_is_custom)
			{
				// yes, this can happen if the instance only has the runnable jar and not the base jar
				// it *could* be assumed that such an instance is vanilla, but that wouldn't be safe
				// because that's not something mmc4 guarantees
				if (runnableJar.isFile() && !baseJar.exists() && modList.empty())
				{
					m_inst->setShouldRebuild(false);
					emitSucceeded();
					return;
				}

				setStatus(tr("Installing mods: Backing up minecraft.jar ..."));
				if (!baseJar.exists() && !QFile::copy(runnableJar.filePath(), baseJar.filePath()))
				{
					emitFailed("It seems both the active and base jar are gone. A fresh base jar will "
							"be used on next run.");
					m_inst->setShouldRebuild(true);
					m_inst->setShouldUpdate(true);
					m_inst->setShouldUseCustomBaseJar(false);
					return;
				}
			}

			if (!baseJar.exists())
			{
				emitFailed("The base jar " + baseJar.filePath() + " does not exist");
				return;
			}

			if (runnableJar.exists() && !QFile::remove(runnableJar.filePath()))
			{
				emitFailed("Failed to delete old minecraft.jar");
				return;
			}

			setStatus(tr("Installing mods: Opening minecraft.jar ..."));

			QString outputJarPath = runnableJar.filePath();
			QString inputJarPath = baseJar.filePath();

			if(!MMCZip::createModdedJar(inputJarPath, outputJarPath, modList))
			{
				emitFailed(tr("Failed to create the custom Minecraft jar file."));
				return;
			}
			m_inst->setShouldRebuild(false);
			// inst->UpdateVersion(true);
			emitSucceeded();
			return;

		}
		std::shared_ptr<LegacyInstance> m_inst;
	};
	return std::make_shared<JarModTask>(std::dynamic_pointer_cast<LegacyInstance>(shared_from_this()));
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
	qDebug() << "Jar mods of instance " << name() << " have changed. Jar will be rebuilt.";
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
	settings()->set("IntendedJarVersion", version);
	setShouldUpdate(true);
	return true;
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

void LegacyInstance::setShouldUpdate(bool val)
{
	settings()->set("ShouldUpdate", val);
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

QString LegacyInstance::lwjglFolder() const
{
	return m_lwjglFolderSetting->get().toString();
}
