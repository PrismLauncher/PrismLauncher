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
#include <pathutils.h>
#include <QDebug>
#include "MMCError.h"

#include "logic/minecraft/OneSixInstance.h"

#include "logic/minecraft/OneSixUpdate.h"
#include "logic/minecraft/MinecraftProfile.h"
#include "logic/minecraft/VersionBuildError.h"
#include "logic/minecraft/MinecraftProcess.h"
#include "logic/minecraft/OneSixProfileStrategy.h"

#include "logic/minecraft/AssetsUtils.h"
#include "logic/icons/IconList.h"
#include "gui/pagedialog/PageDialog.h"
#include "gui/pages/VersionPage.h"
#include "gui/pages/ModFolderPage.h"
#include "gui/pages/ResourcePackPage.h"
#include "gui/pages/TexturePackPage.h"
#include "gui/pages/InstanceSettingsPage.h"
#include "gui/pages/NotesPage.h"
#include "gui/pages/ScreenshotsPage.h"
#include "gui/pages/OtherLogsPage.h"
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


QList<BasePage *> OneSixInstance::getPages()
{
	QList<BasePage *> values;
	values.append(new VersionPage(this));
	values.append(new ModFolderPage(this, loaderModList(), "mods", "loadermods",
									tr("Loader mods"), "Loader-mods"));
	values.append(new CoreModFolderPage(this, coreModList(), "coremods", "coremods",
										tr("Core mods"), "Core-mods"));
	values.append(new ResourcePackPage(this));
	values.append(new TexturePackPage(this));
	values.append(new NotesPage(this));
	values.append(new ScreenshotsPage(PathCombine(minecraftRoot(), "screenshots")));
	values.append(new InstanceSettingsPage(this));
	values.append(new OtherLogsPage(minecraftRoot()));
	return values;
}

QString OneSixInstance::dialogTitle()
{
	return tr("Edit Instance (%1)").arg(name());
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

std::shared_ptr<Task> OneSixInstance::doUpdate()
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

	QString absRootDir = QDir(minecraftRoot()).absolutePath();
	token_mapping["game_directory"] = absRootDir;
	QString absAssetsDir = QDir("assets/").absolutePath();
	token_mapping["game_assets"] = AssetsUtils::reconstructAssets(m_version->assets).absolutePath();

	token_mapping["user_properties"] = session->serializeUserProperties();
	token_mapping["user_type"] = session->user_type;
	// 1.7.3+ assets tokens
	token_mapping["assets_root"] = absAssetsDir;
	token_mapping["assets_index_name"] = m_version->assets;

	QStringList parts = args_pattern.split(' ', QString::SkipEmptyParts);
	for (int i = 0; i < parts.length(); i++)
	{
		parts[i] = replaceTokensIn(parts[i], token_mapping);
	}
	return parts;
}

BaseProcess *OneSixInstance::prepareForLaunch(AuthSessionPtr session)
{
	QString launchScript;
	QIcon icon = ENV.icons()->getIcon(iconKey());
	auto pixmap = icon.pixmap(128, 128);
	pixmap.save(PathCombine(minecraftRoot(), "icon.png"), "PNG");

	if (!m_version)
		return nullptr;

	// libraries and class path.
	{
		auto libs = m_version->getActiveNormalLibs();
		for (auto lib : libs)
		{
			launchScript += "cp " + librariesPath().absoluteFilePath(lib->storagePath()) + "\n";
		}
		auto jarMods = getJarMods();
		if (!jarMods.isEmpty())
		{
			launchScript += "cp " + QDir(instanceRoot()).absoluteFilePath("temp.jar") + "\n";
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
		if (settings().get("LaunchMaximized").toBool())
			windowParams = "max";
		else
			windowParams = QString("%1x%2")
							   .arg(settings().get("MinecraftWinWidth").toInt())
							   .arg(settings().get("MinecraftWinHeight").toInt());
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
		QDir natives_dir(PathCombine(instanceRoot(), "natives/"));
		for (auto native : m_version->getActiveNativeLibs())
		{
			QFileInfo finfo(PathCombine("libraries", native->storagePath()));
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

	auto process = MinecraftProcess::create(std::dynamic_pointer_cast<MinecraftInstance>(getSharedPtr()));
	process->setLaunchScript(launchScript);
	process->setWorkdir(minecraftRoot());
	process->setLogin(session);
	return process;
}

void OneSixInstance::cleanupAfterRun()
{
	QString target_dir = PathCombine(instanceRoot(), "natives/");
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

bool OneSixInstance::setIntendedVersionId(QString version)
{
	settings().set("IntendedVersion", version);
	if(getMinecraftProfile())
	{
		clearProfile();
	}
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
	return settings().get("IntendedVersion").toString();
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
	catch (MMCError &error)
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

QString OneSixInstance::getStatusbarDescription()
{
	QStringList traits;
	if (flags() & VersionBrokenFlag)
	{
		traits.append(tr("broken"));
	}

	if (traits.size())
	{
		return tr("Minecraft %1 (%2)").arg(intendedVersionId()).arg(traits.join(", "));
	}
	else
	{
		return tr("Minecraft %1").arg(intendedVersionId());
	}
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
	return PathCombine(minecraftRoot(), "mods");
}

QString OneSixInstance::coreModsDir() const
{
	return PathCombine(minecraftRoot(), "coremods");
}

QString OneSixInstance::resourcePacksDir() const
{
	return PathCombine(minecraftRoot(), "resourcepacks");
}

QString OneSixInstance::texturePacksDir() const
{
	return PathCombine(minecraftRoot(), "texturepacks");
}

QString OneSixInstance::instanceConfigFolder() const
{
	return PathCombine(minecraftRoot(), "config");
}

QString OneSixInstance::jarModsDir() const
{
	return PathCombine(instanceRoot(), "jarmods");
}

QString OneSixInstance::libDir() const
{
	return PathCombine(minecraftRoot(), "lib");
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
