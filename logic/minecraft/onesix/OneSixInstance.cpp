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

#include <QIcon>
#include <QDebug>

#include "OneSixInstance.h"
#include "OneSixUpdate.h"
#include "OneSixProfileStrategy.h"

#include "minecraft/MinecraftProfile.h"
#include "minecraft/VersionBuildError.h"
#include "launch/LaunchTask.h"
#include "launch/steps/PreLaunchCommand.h"
#include "launch/steps/Update.h"
#include "launch/steps/LaunchMinecraft.h"
#include "launch/steps/PostLaunchCommand.h"
#include "launch/steps/TextPrint.h"
#include "launch/steps/ModMinecraftJar.h"
#include "launch/steps/CheckJava.h"
#include "MMCZip.h"

#include "minecraft/AssetsUtils.h"
#include "icons/IconList.h"
#include "minecraft/WorldList.h"
#include <FileSystem.h>

OneSixInstance::OneSixInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
	: MinecraftInstance(globalSettings, settings, rootDir)
{
	m_settings->registerSetting({"IntendedVersion", "MinecraftVersion"}, "");
}

void OneSixInstance::init()
{
	createProfile();
}

void OneSixInstance::createProfile()
{
	m_version.reset(new MinecraftProfile(new OneSixProfileStrategy(this)));
}

QSet<QString> OneSixInstance::traits()
{
	auto version = getMinecraftProfile();
	if (!version)
	{
		return {"version-incomplete"};
	}
	else
		return version->traits;
}

std::shared_ptr<Task> OneSixInstance::createUpdateTask()
{
	return std::shared_ptr<Task>(new OneSixUpdate(this));
}

QString replaceTokensIn(QString text, QMap<QString, QString> with)
{
	QString result;
	QRegExp token_regexp("\\$\\{(.+)\\}");
	token_regexp.setMinimal(true);
	QStringList list;
	int tail = 0;
	int head = 0;
	while ((head = token_regexp.indexIn(text, head)) != -1)
	{
		result.append(text.mid(tail, head - tail));
		QString key = token_regexp.cap(1);
		auto iter = with.find(key);
		if (iter != with.end())
		{
			result.append(*iter);
		}
		head += token_regexp.matchedLength();
		tail = head;
	}
	result.append(text.mid(tail));
	return result;
}

QStringList OneSixInstance::processMinecraftArgs(AuthSessionPtr session)
{
	QString args_pattern = m_version->minecraftArguments;
	for (auto tweaker : m_version->tweakers)
	{
		args_pattern += " --tweakClass " + tweaker;
	}

	QMap<QString, QString> token_mapping;
	// yggdrasil!
	token_mapping["auth_username"] = session->username;
	token_mapping["auth_session"] = session->session;
	token_mapping["auth_access_token"] = session->access_token;
	token_mapping["auth_player_name"] = session->player_name;
	token_mapping["auth_uuid"] = session->uuid;

	// blatant self-promotion.
	token_mapping["profile_name"] = token_mapping["version_name"] = "MultiMC5";
	if(m_version->isVanilla())
	{
		token_mapping["version_type"] = m_version->type;
	}
	else
	{
		token_mapping["version_type"] = "custom";
	}

	QString absRootDir = QDir(minecraftRoot()).absolutePath();
	token_mapping["game_directory"] = absRootDir;
	QString absAssetsDir = QDir("assets/").absolutePath();
	token_mapping["game_assets"] = AssetsUtils::reconstructAssets(m_version->assets).absolutePath();

	token_mapping["user_properties"] = session->serializeUserProperties();
	token_mapping["user_type"] = session->user_type;

	// 1.7.3+ assets tokens
	token_mapping["assets_root"] = absAssetsDir;
	token_mapping["assets_index_name"] = m_version->assets;

	// 1.9+ version type token
	if(m_version->isVanilla())
	{
		token_mapping["version_type"] = m_version->type;
	}
	else
	{
		token_mapping["version_type"] = "custom";
	}

	QStringList parts = args_pattern.split(' ', QString::SkipEmptyParts);
	for (int i = 0; i < parts.length(); i++)
	{
		parts[i] = replaceTokensIn(parts[i], token_mapping);
	}
	return parts;
}

std::shared_ptr<LaunchTask> OneSixInstance::createLaunchTask(AuthSessionPtr session)
{
	QString launchScript;
	QIcon icon = ENV.icons()->getIcon(iconKey());
	auto pixmap = icon.pixmap(128, 128);
	pixmap.save(FS::PathCombine(minecraftRoot(), "icon.png"), "PNG");

	if (!m_version)
		return nullptr;

	for(auto & mod: loaderModList()->allMods())
	{
		if(!mod.enabled())
			continue;
		if(mod.type() == Mod::MOD_FOLDER)
			continue;
		// TODO: proper implementation would need to descend into folders.

		launchScript += "mod " + mod.filename().completeBaseName()  + "\n";;
	}

	for(auto & coremod: coreModList()->allMods())
	{
		if(!coremod.enabled())
			continue;
		if(coremod.type() == Mod::MOD_FOLDER)
			continue;
		// TODO: proper implementation would need to descend into folders.

		launchScript += "coremod " + coremod.filename().completeBaseName()  + "\n";;
	}

	for(auto & jarmod: m_version->jarMods)
	{
		launchScript += "jarmod " + jarmod->originalName + " (" + jarmod->name + ")\n";
	}

	// libraries and class path.
	{
		auto libs = m_version->getActiveNormalLibs();
		for (auto lib : libs)
		{
			launchScript += "cp " + QFileInfo(lib->storagePath()).absoluteFilePath() + "\n";
		}
		auto jarMods = getJarMods();
		if (!jarMods.isEmpty())
		{
			launchScript += "cp " + QDir(instanceRoot()).absoluteFilePath("minecraft.jar") + "\n";
		}
		else
		{
			QString relpath = m_version->id + "/" + m_version->id + ".jar";
			launchScript += "cp " + versionsPath().absoluteFilePath(relpath) + "\n";
		}
	}
	if (!m_version->mainClass.isEmpty())
	{
		launchScript += "mainClass " + m_version->mainClass + "\n";
	}
	if (!m_version->appletClass.isEmpty())
	{
		launchScript += "appletClass " + m_version->appletClass + "\n";
	}

	// generic minecraft params
	for (auto param : processMinecraftArgs(session))
	{
		launchScript += "param " + param + "\n";
	}

	// window size, title and state, legacy
	{
		QString windowParams;
		if (settings()->get("LaunchMaximized").toBool())
			windowParams = "max";
		else
			windowParams = QString("%1x%2")
							   .arg(settings()->get("MinecraftWinWidth").toInt())
							   .arg(settings()->get("MinecraftWinHeight").toInt());
		launchScript += "windowTitle " + windowTitle() + "\n";
		launchScript += "windowParams " + windowParams + "\n";
	}

	// legacy auth
	{
		launchScript += "userName " + session->player_name + "\n";
		launchScript += "sessionId " + session->session + "\n";
	}

	// native libraries (mostly LWJGL)
	{
		QDir natives_dir(FS::PathCombine(instanceRoot(), "natives/"));
		for (auto native : m_version->getActiveNativeLibs())
		{
			QFileInfo finfo(native->storagePath());
			launchScript += "ext " + finfo.absoluteFilePath() + "\n";
		}
		launchScript += "natives " + natives_dir.absolutePath() + "\n";
	}

	// traits. including legacyLaunch and others ;)
	for (auto trait : m_version->traits)
	{
		launchScript += "traits " + trait + "\n";
	}
	launchScript += "launcher onesix\n";

	auto process = LaunchTask::create(std::dynamic_pointer_cast<MinecraftInstance>(getSharedPtr()));
	auto pptr = process.get();

	// print a header
	{
		process->appendStep(std::make_shared<TextPrint>(pptr, "Minecraft folder is:\n" + minecraftRoot() + "\n\n", MessageLevel::MultiMC));
	}
	{
		auto step = std::make_shared<CheckJava>(pptr);
		process->appendStep(step);
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

std::shared_ptr<Task> OneSixInstance::createJarModdingTask()
{
	class JarModTask : public Task
	{
	public:
		explicit JarModTask(std::shared_ptr<OneSixInstance> inst) : Task(nullptr), m_inst(inst)
		{
		}
		virtual void executeTask()
		{
			std::shared_ptr<MinecraftProfile> version = m_inst->getMinecraftProfile();
			// nuke obsolete stripped jar(s) if needed
			QString version_id = version->id;
			QString strippedPath = version_id + "/" + version_id + "-stripped.jar";
			QFile strippedJar(strippedPath);
			if(strippedJar.exists())
			{
				strippedJar.remove();
			}
			auto tempJarPath = QDir(m_inst->instanceRoot()).absoluteFilePath("temp.jar");
			QFile tempJar(tempJarPath);
			if(tempJar.exists())
			{
				tempJar.remove();
			}
			auto finalJarPath = QDir(m_inst->instanceRoot()).absoluteFilePath("minecraft.jar");
			QFile finalJar(finalJarPath);
			if(finalJar.exists())
			{
				if(!finalJar.remove())
				{
					emitFailed(tr("Couldn't remove stale jar file: %1").arg(finalJarPath));
					return;
				}
			}

			// create temporary modded jar, if needed
			auto jarMods = m_inst->getJarMods();
			if(jarMods.size())
			{
				auto sourceJarPath = m_inst->versionsPath().absoluteFilePath(version->id + "/" + version->id + ".jar");
				QString localPath = version_id + "/" + version_id + ".jar";
				auto metacache = ENV.metacache();
				auto entry = metacache->resolveEntry("versions", localPath);
				QString fullJarPath = entry->getFullPath();
				if(!MMCZip::createModdedJar(sourceJarPath, finalJarPath, jarMods))
				{
					emitFailed(tr("Failed to create the custom Minecraft jar file."));
					return;
				}
			}
			emitSucceeded();
		}
		std::shared_ptr<OneSixInstance> m_inst;
	};
	return std::make_shared<JarModTask>(std::dynamic_pointer_cast<OneSixInstance>(shared_from_this()));
}

void OneSixInstance::cleanupAfterRun()
{
	QString target_dir = FS::PathCombine(instanceRoot(), "natives/");
	QDir dir(target_dir);
	dir.removeRecursively();
}

std::shared_ptr<ModList> OneSixInstance::loaderModList() const
{
	if (!m_loader_mod_list)
	{
		m_loader_mod_list.reset(new ModList(loaderModsDir()));
	}
	m_loader_mod_list->update();
	return m_loader_mod_list;
}

std::shared_ptr<ModList> OneSixInstance::coreModList() const
{
	if (!m_core_mod_list)
	{
		m_core_mod_list.reset(new ModList(coreModsDir()));
	}
	m_core_mod_list->update();
	return m_core_mod_list;
}

std::shared_ptr<ModList> OneSixInstance::resourcePackList() const
{
	if (!m_resource_pack_list)
	{
		m_resource_pack_list.reset(new ModList(resourcePacksDir()));
	}
	m_resource_pack_list->update();
	return m_resource_pack_list;
}

std::shared_ptr<ModList> OneSixInstance::texturePackList() const
{
	if (!m_texture_pack_list)
	{
		m_texture_pack_list.reset(new ModList(texturePacksDir()));
	}
	m_texture_pack_list->update();
	return m_texture_pack_list;
}

std::shared_ptr<WorldList> OneSixInstance::worldList() const
{
	if (!m_world_list)
	{
		m_world_list.reset(new WorldList(worldDir()));
	}
	return m_world_list;
}

bool OneSixInstance::setIntendedVersionId(QString version)
{
	settings()->set("IntendedVersion", version);
	if(getMinecraftProfile())
	{
		clearProfile();
	}
	emit propertiesChanged(this);
	return true;
}

QList< Mod > OneSixInstance::getJarMods() const
{
	QList<Mod> mods;
	for (auto jarmod : m_version->jarMods)
	{
		QString filePath = jarmodsPath().absoluteFilePath(jarmod->name);
		mods.push_back(Mod(QFileInfo(filePath)));
	}
	return mods;
}


QString OneSixInstance::intendedVersionId() const
{
	return settings()->get("IntendedVersion").toString();
}

void OneSixInstance::setShouldUpdate(bool)
{
}

bool OneSixInstance::shouldUpdate() const
{
	return true;
}

QString OneSixInstance::currentVersionId() const
{
	return intendedVersionId();
}

void OneSixInstance::reloadProfile()
{
	try
	{
		m_version->reload();
		unsetFlag(VersionBrokenFlag);
		emit versionReloaded();
	}
	catch (VersionIncomplete &error)
	{
	}
	catch (Exception &error)
	{
		m_version->clear();
		setFlag(VersionBrokenFlag);
		// TODO: rethrow to show some error message(s)?
		emit versionReloaded();
		throw;
	}
}

void OneSixInstance::clearProfile()
{
	m_version->clear();
	emit versionReloaded();
}

std::shared_ptr<MinecraftProfile> OneSixInstance::getMinecraftProfile() const
{
	return m_version;
}

QDir OneSixInstance::librariesPath() const
{
	return QDir::current().absoluteFilePath("libraries");
}

QDir OneSixInstance::jarmodsPath() const
{
	return QDir(jarModsDir());
}

QDir OneSixInstance::versionsPath() const
{
	return QDir::current().absoluteFilePath("versions");
}

bool OneSixInstance::providesVersionFile() const
{
	return false;
}

bool OneSixInstance::reload()
{
	if (BaseInstance::reload())
	{
		try
		{
			reloadProfile();
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	return false;
}

QString OneSixInstance::loaderModsDir() const
{
	return FS::PathCombine(minecraftRoot(), "mods");
}

QString OneSixInstance::coreModsDir() const
{
	return FS::PathCombine(minecraftRoot(), "coremods");
}

QString OneSixInstance::resourcePacksDir() const
{
	return FS::PathCombine(minecraftRoot(), "resourcepacks");
}

QString OneSixInstance::texturePacksDir() const
{
	return FS::PathCombine(minecraftRoot(), "texturepacks");
}

QString OneSixInstance::instanceConfigFolder() const
{
	return FS::PathCombine(minecraftRoot(), "config");
}

QString OneSixInstance::jarModsDir() const
{
	return FS::PathCombine(instanceRoot(), "jarmods");
}

QString OneSixInstance::libDir() const
{
	return FS::PathCombine(minecraftRoot(), "lib");
}

QString OneSixInstance::worldDir() const
{
	return FS::PathCombine(minecraftRoot(), "saves");
}

QStringList OneSixInstance::extraArguments() const
{
	auto list = BaseInstance::extraArguments();
	auto version = getMinecraftProfile();
	if (!version)
		return list;
	auto jarMods = getJarMods();
	if (!jarMods.isEmpty())
	{
		list.append({"-Dfml.ignoreInvalidMinecraftCertificates=true",
					 "-Dfml.ignorePatchDiscrepancies=true"});
	}
	return list;
}

std::shared_ptr<OneSixInstance> OneSixInstance::getSharedPtr()
{
	return std::dynamic_pointer_cast<OneSixInstance>(BaseInstance::getSharedPtr());
}

QString OneSixInstance::typeName() const
{
	return tr("OneSix");
}
