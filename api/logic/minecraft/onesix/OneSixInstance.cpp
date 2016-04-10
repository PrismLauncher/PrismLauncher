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

#include <QDebug>
#include <Env.h>

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
	m_profile.reset(new MinecraftProfile(new OneSixProfileStrategy(this)));
}

QSet<QString> OneSixInstance::traits()
{
	auto version = getMinecraftProfile();
	if (!version)
	{
		return {"version-incomplete"};
	}
	else
	{
		return version->getTraits();
	}
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
	QString args_pattern = m_profile->getMinecraftArguments();
	for (auto tweaker : m_profile->getTweakers())
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
	if(m_profile->isVanilla())
	{
		token_mapping["version_type"] = m_profile->getMinecraftVersionType();
	}
	else
	{
		token_mapping["version_type"] = "custom";
	}

	QString absRootDir = QDir(minecraftRoot()).absolutePath();
	token_mapping["game_directory"] = absRootDir;
	QString absAssetsDir = QDir("assets/").absolutePath();
	auto assets = m_profile->getMinecraftAssets();
	token_mapping["game_assets"] = AssetsUtils::reconstructAssets(assets->id).absolutePath();

	token_mapping["user_properties"] = session->serializeUserProperties();
	token_mapping["user_type"] = session->user_type;

	// 1.7.3+ assets tokens
	token_mapping["assets_root"] = absAssetsDir;
	token_mapping["assets_index_name"] = assets->id;

	QStringList parts = args_pattern.split(' ', QString::SkipEmptyParts);
	for (int i = 0; i < parts.length(); i++)
	{
		parts[i] = replaceTokensIn(parts[i], token_mapping);
	}
	return parts;
}

QString OneSixInstance::createLaunchScript(AuthSessionPtr session)
{
	QString launchScript;

	if (!m_profile)
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

	for(auto & jarmod: m_profile->getJarMods())
	{
		launchScript += "jarmod " + jarmod->originalName + " (" + jarmod->name + ")\n";
	}

	auto mainClass = m_profile->getMainClass();
	if (!mainClass.isEmpty())
	{
		launchScript += "mainClass " + mainClass + "\n";
	}
	auto appletClass = m_profile->getAppletClass();
	if (!appletClass.isEmpty())
	{
		launchScript += "appletClass " + appletClass + "\n";
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

	// libraries and class path.
	{
		auto libs = m_profile->getLibraries();

		QStringList jar, native, native32, native64;
		for (auto lib : libs)
		{
			lib->getApplicableFiles(currentSystem, jar, native, native32, native64);
		}
		for(auto file: jar)
		{
			launchScript += "cp " + file + "\n";
		}
		for(auto file: native)
		{
			launchScript += "ext " + file + "\n";
		}
		for(auto file: native32)
		{
			launchScript += "ext32 " + file + "\n";
		}
		for(auto file: native64)
		{
			launchScript += "ext64 " + file + "\n";
		}
		QDir natives_dir(FS::PathCombine(instanceRoot(), "natives/"));
		launchScript += "natives " + natives_dir.absolutePath() + "\n";
		auto jarMods = getJarMods();
		if (!jarMods.isEmpty())
		{
			launchScript += "cp " + QDir(instanceRoot()).absoluteFilePath("minecraft.jar") + "\n";
		}
		else
		{
			QString relpath = m_profile->getMinecraftVersion() + "/" + m_profile->getMinecraftVersion() + ".jar";
			launchScript += "cp " + versionsPath().absoluteFilePath(relpath) + "\n";
		}
	}

	// traits. including legacyLaunch and others ;)
	for (auto trait : m_profile->getTraits())
	{
		launchScript += "traits " + trait + "\n";
	}
	launchScript += "launcher onesix\n";
	return launchScript;
}

std::shared_ptr<LaunchTask> OneSixInstance::createLaunchTask(AuthSessionPtr session)
{
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
		step->setAuthSession(session);
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
			auto profile = m_inst->getMinecraftProfile();
			// nuke obsolete stripped jar(s) if needed
			QString version_id = profile->getMinecraftVersion();
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
				auto sourceJarPath = m_inst->versionsPath().absoluteFilePath(version_id + "/" + version_id + ".jar");
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
	for (auto jarmod : m_profile->getJarMods())
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
	m_profile->reload();
	auto severity = m_profile->getProblemSeverity();
	if(severity == ProblemSeverity::PROBLEM_ERROR)
	{
		setFlag(VersionBrokenFlag);
	}
	else
	{
		unsetFlag(VersionBrokenFlag);
	}
	emit versionReloaded();
}

void OneSixInstance::clearProfile()
{
	m_profile->clear();
	emit versionReloaded();
}

std::shared_ptr<MinecraftProfile> OneSixInstance::getMinecraftProfile() const
{
	return m_profile;
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
