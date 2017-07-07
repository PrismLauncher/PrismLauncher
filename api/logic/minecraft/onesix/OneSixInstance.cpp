/* Copyright 2013-2017 MultiMC Contributors
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
#include <minecraft/launch/DirectJavaLaunch.h>
#include <minecraft/launch/LauncherPartLaunch.h>
#include <Env.h>

#include "OneSixInstance.h"
#include "OneSixUpdate.h"
#include "OneSixProfileStrategy.h"

#include "minecraft/MinecraftProfile.h"
#include "minecraft/launch/ModMinecraftJar.h"
#include "MMCZip.h"

#include "minecraft/AssetsUtils.h"
#include "minecraft/WorldList.h"
#include <FileSystem.h>

OneSixInstance::OneSixInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
	: MinecraftInstance(globalSettings, settings, rootDir)
{
	// set explicitly during instance creation
	m_settings->registerSetting({"IntendedVersion", "MinecraftVersion"}, "");

	// defaults to the version we've been using for years (2.9.1)
	m_settings->registerSetting("LWJGLVersion", "2.9.1");

	// optionals
	m_settings->registerSetting("ForgeVersion", "");
	m_settings->registerSetting("LiteloaderVersion", "");
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

shared_qobject_ptr<Task> OneSixInstance::createUpdateTask()
{
	return shared_qobject_ptr<Task>(new OneSixUpdate(this));
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

QStringList OneSixInstance::processMinecraftArgs(AuthSessionPtr session) const
{
	QString args_pattern = m_profile->getMinecraftArguments();
	for (auto tweaker : m_profile->getTweakers())
	{
		args_pattern += " --tweakClass " + tweaker;
	}

	QMap<QString, QString> token_mapping;
	// yggdrasil!
	if(session)
	{
		token_mapping["auth_username"] = session->username;
		token_mapping["auth_session"] = session->session;
		token_mapping["auth_access_token"] = session->access_token;
		token_mapping["auth_player_name"] = session->player_name;
		token_mapping["auth_uuid"] = session->uuid;
		token_mapping["user_properties"] = session->serializeUserProperties();
		token_mapping["user_type"] = session->user_type;
	}

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

QString OneSixInstance::getNativePath() const
{
	QDir natives_dir(FS::PathCombine(instanceRoot(), "natives/"));
	return natives_dir.absolutePath();
}

QString OneSixInstance::getLocalLibraryPath() const
{
	QDir libraries_dir(FS::PathCombine(instanceRoot(), "libraries/"));
	return libraries_dir.absolutePath();
}

QString OneSixInstance::createLaunchScript(AuthSessionPtr session)
{
	QString launchScript;

	if (!m_profile)
		return nullptr;

	auto mainClass = getMainClass();
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
	if(session)
	{
		launchScript += "userName " + session->player_name + "\n";
		launchScript += "sessionId " + session->session + "\n";
	}

	// libraries and class path.
	{
		QStringList jars, nativeJars;
		auto javaArchitecture = settings()->get("JavaArchitecture").toString();
		m_profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
		for(auto file: jars)
		{
			launchScript += "cp " + file + "\n";
		}
		for(auto file: nativeJars)
		{
			launchScript += "ext " + file + "\n";
		}
		launchScript += "natives " + getNativePath() + "\n";
	}

	for (auto trait : m_profile->getTraits())
	{
		launchScript += "traits " + trait + "\n";
	}
	launchScript += "launcher onesix\n";
	return launchScript;
}

QStringList OneSixInstance::verboseDescription(AuthSessionPtr session)
{
	QStringList out;
	out << "Main Class:" << "  " + getMainClass() << "";
	out << "Native path:" << "  " + getNativePath() << "";


	auto alltraits = traits();
	if(alltraits.size())
	{
		out << "Traits:";
		for (auto trait : alltraits)
		{
			out << "traits " + trait;
		}
		out << "";
	}

	// libraries and class path.
	{
		out << "Libraries:";
		QStringList jars, nativeJars;
		auto javaArchitecture = settings()->get("JavaArchitecture").toString();
		m_profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
		auto printLibFile = [&](const QString & path)
		{
			QFileInfo info(path);
			if(info.exists())
			{
				out << "  " + path;
			}
			else
			{
				out << "  " + path + " (missing)";
			}
		};
		for(auto file: jars)
		{
			printLibFile(file);
		}
		out << "";
		out << "Native libraries:";
		for(auto file: nativeJars)
		{
			printLibFile(file);
		}
		out << "";
	}

	if(loaderModList()->size())
	{
		out << "Mods:";
		for(auto & mod: loaderModList()->allMods())
		{
			if(!mod.enabled())
				continue;
			if(mod.type() == Mod::MOD_FOLDER)
				continue;
			// TODO: proper implementation would need to descend into folders.

			out << "  " + mod.filename().completeBaseName();
		}
		out << "";
	}

	if(coreModList()->size())
	{
		out << "Core Mods:";
		for(auto & coremod: coreModList()->allMods())
		{
			if(!coremod.enabled())
				continue;
			if(coremod.type() == Mod::MOD_FOLDER)
				continue;
			// TODO: proper implementation would need to descend into folders.

			out << "  " + coremod.filename().completeBaseName();
		}
		out << "";
	}

	auto & jarMods = m_profile->getJarMods();
	if(jarMods.size())
	{
		out << "Jar Mods:";
		for(auto & jarmod: jarMods)
		{
			auto displayname = jarmod->displayName(currentSystem);
			auto realname = jarmod->filename(currentSystem);
			if(displayname != realname)
			{
				out << "  " + displayname + " (" + realname + ")";
			}
			else
			{
				out << "  " + realname;
			}
		}
		out << "";
	}

	auto params = processMinecraftArgs(nullptr);
	out << "Params:";
	out << "  " + params.join(' ');
	out << "";

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


std::shared_ptr<LaunchStep> OneSixInstance::createMainLaunchStep(LaunchTask * parent, AuthSessionPtr session)
{
	auto method = launchMethod();
	if(method == "LauncherPart")
	{
		auto step = std::make_shared<LauncherPartLaunch>(parent);
		step->setAuthSession(session);
		step->setWorkingDirectory(minecraftRoot());
		return step;
	}
	else if (method == "DirectJava")
	{
		auto step = std::make_shared<DirectJavaLaunch>(parent);
		step->setWorkingDirectory(minecraftRoot());
		step->setAuthSession(session);
		return step;
	}
	return nullptr;
}

class JarModTask : public Task
{
	Q_OBJECT
public:
	explicit JarModTask(std::shared_ptr<OneSixInstance> inst) : Task(nullptr), m_inst(inst)
	{
	}
	virtual void executeTask()
	{
		auto profile = m_inst->getMinecraftProfile();
		// nuke obsolete stripped jar(s) if needed
		QString version_id = profile->getMinecraftVersion();
		if(!FS::ensureFolderPathExists(m_inst->binRoot()))
		{
			emitFailed(tr("Couldn't create the bin folder for Minecraft.jar"));
		}
		auto finalJarPath = QDir(m_inst->binRoot()).absoluteFilePath("minecraft.jar");
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
			auto mainJar = profile->getMainJar();
			QStringList jars, temp1, temp2, temp3, temp4;
			mainJar->getApplicableFiles(currentSystem, jars, temp1, temp2, temp3, m_inst->getLocalLibraryPath());
			auto sourceJarPath = jars[0];
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

std::shared_ptr<Task> OneSixInstance::createJarModdingTask()
{
	return std::make_shared<JarModTask>(std::dynamic_pointer_cast<OneSixInstance>(shared_from_this()));
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
	return setComponentVersion("net.minecraft", version);
}

QString OneSixInstance::intendedVersionId() const
{
	return getComponentVersion("net.minecraft");
}

bool OneSixInstance::setComponentVersion(const QString& uid, const QString& version)
{
	if(uid == "net.minecraft")
	{
		settings()->set("IntendedVersion", version);
	}
	else if (uid == "org.lwjgl")
	{
		settings()->set("LWJGLVersion", version);
	}
	else if (uid == "net.minecraftforge")
	{
		settings()->set("ForgeVersion", version);
	}
	else if (uid == "com.mumfrey.liteloader")
	{
		settings()->set("LiteloaderVersion", version);
	}
	if(getMinecraftProfile())
	{
		clearProfile();
	}
	emit propertiesChanged(this);
	return true;
}

QString OneSixInstance::getComponentVersion(const QString& uid) const
{
	if(uid == "net.minecraft")
	{
		return settings()->get("IntendedVersion").toString();
	}
	else if(uid == "org.lwjgl")
	{
		return settings()->get("LWJGLVersion").toString();
	}
	else if(uid == "net.minecraftforge")
	{
		return settings()->get("ForgeVersion").toString();
	}
	else if(uid == "com.mumfrey.liteloader")
	{
		return settings()->get("LiteloaderVersion").toString();
	}
	return QString();
}

QList< Mod > OneSixInstance::getJarMods() const
{
	QList<Mod> mods;
	for (auto jarmod : m_profile->getJarMods())
	{
		QStringList jar, temp1, temp2, temp3;
		jarmod->getApplicableFiles(currentSystem, jar, temp1, temp2, temp3, jarmodsPath().absolutePath());
		// QString filePath = jarmodsPath().absoluteFilePath(jarmod->filename(currentSystem));
		mods.push_back(Mod(QFileInfo(jar[0])));
	}
	return mods;
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
	setVersionBroken(m_profile->getProblemSeverity() == ProblemSeverity::Error);
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

QStringList OneSixInstance::validLaunchMethods()
{
	return {"LauncherPart", "DirectJava"};
}

QStringList OneSixInstance::getClassPath() const
{
	QStringList jars, nativeJars;
	auto javaArchitecture = settings()->get("JavaArchitecture").toString();
	m_profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
	return jars;
}

QString OneSixInstance::getMainClass() const
{
	return m_profile->getMainClass();
}

QStringList OneSixInstance::getNativeJars() const
{
	QStringList jars, nativeJars;
	auto javaArchitecture = settings()->get("JavaArchitecture").toString();
	m_profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
	return nativeJars;
}

#include "OneSixInstance.moc"
