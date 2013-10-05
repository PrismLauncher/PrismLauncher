#include "OneSixInstance.h"
#include "OneSixInstance_p.h"
#include "OneSixUpdate.h"
#include "MinecraftProcess.h"
#include "OneSixVersion.h"

#include <setting.h>
#include <pathutils.h>
#include <cmdutils.h>
#include <JlCompress.h>
#include <gui/OneSixModEditDialog.h>
#include <logger/QsLog.h>

OneSixInstance::OneSixInstance(const QString &rootDir, SettingsObject *setting_obj,
							   QObject *parent)
	: BaseInstance(new OneSixInstancePrivate(), rootDir, setting_obj, parent)
{
	I_D(OneSixInstance);
	d->m_settings->registerSetting(new Setting("IntendedVersion", ""));
	d->m_settings->registerSetting(new Setting("ShouldUpdate", false));
	reloadFullVersion();
}

BaseUpdate *OneSixInstance::doUpdate()
{
	return new OneSixUpdate(this);
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

QStringList OneSixInstance::processMinecraftArgs(LoginResponse response)
{
	I_D(OneSixInstance);
	auto version = d->version;
	QString args_pattern = version->minecraftArguments;

	QMap<QString, QString> token_mapping;
	token_mapping["auth_username"] = response.username;
	token_mapping["auth_session"] = response.session_id;
	token_mapping["auth_player_name"] = response.player_name;
	token_mapping["auth_uuid"] = response.player_id;

	// this is for offline?:
	/*
	map["auth_player_name"] = "Player";
	map["auth_player_name"] = "00000000-0000-0000-0000-000000000000";
	*/

	token_mapping["profile_name"] = name();
	token_mapping["version_name"] = version->id;

	QString absRootDir = QDir(minecraftRoot()).absolutePath();
	token_mapping["game_directory"] = absRootDir;
	QString absAssetsDir = QDir("assets/").absolutePath();
	token_mapping["game_assets"] = absAssetsDir;

	QStringList parts = args_pattern.split(' ', QString::SkipEmptyParts);
	for (int i = 0; i < parts.length(); i++)
	{
		parts[i] = replaceTokensIn(parts[i], token_mapping);
	}
	return parts;
}

MinecraftProcess *OneSixInstance::prepareForLaunch(LoginResponse response)
{
	I_D(OneSixInstance);
	cleanupAfterRun();
	auto version = d->version;
	if (!version)
		return nullptr;
	auto libs_to_extract = version->getActiveNativeLibs();
	QString natives_dir_raw = PathCombine(instanceRoot(), "natives/");
	bool success = ensureFolderPathExists(natives_dir_raw);
	if (!success)
	{
		// FIXME: handle errors
		return nullptr;
	}

	for (auto lib : libs_to_extract)
	{
		QString path = "libraries/" + lib->storagePath();
		QLOG_INFO() << "Will extract " << path.toLocal8Bit();
		if (JlCompress::extractWithExceptions(path, natives_dir_raw, lib->extract_excludes)
				.isEmpty())
		{
			return nullptr;
		}
	}

	QStringList args;
	args.append(Util::Commandline::splitArgs(settings().get("JvmArgs").toString()));
	args << QString("-Xms%1m").arg(settings().get("MinMemAlloc").toInt());
	args << QString("-Xmx%1m").arg(settings().get("MaxMemAlloc").toInt());
	args << QString("-XX:PermSize=%1m").arg(settings().get("PermGen").toInt());
	QDir natives_dir(natives_dir_raw);
	args << QString("-Djava.library.path=%1").arg(natives_dir.absolutePath());
	QString classPath;
	{
		auto libs = version->getActiveNormalLibs();
		for (auto lib : libs)
		{
			QFileInfo fi(QString("libraries/") + lib->storagePath());
			classPath.append(fi.absoluteFilePath());
#ifdef Q_OS_WIN32
			classPath.append(';');
#else
			classPath.append(':');
#endif
		}
		QString targetstr = "versions/" + version->id + "/" + version->id + ".jar";
		QFileInfo fi(targetstr);
		classPath.append(fi.absoluteFilePath());
	}
	if (classPath.size())
	{
		args << "-cp";
		args << classPath;
	}
	args << version->mainClass;
	args.append(processMinecraftArgs(response));

	// create the process and set its parameters
	MinecraftProcess *proc = new MinecraftProcess(this);
	proc->setMinecraftArguments(args);
	proc->setMinecraftWorkdir(minecraftRoot());
	return proc;
}

void OneSixInstance::cleanupAfterRun()
{
	QString target_dir = PathCombine(instanceRoot(), "natives/");
	QDir dir(target_dir);
	dir.removeRecursively();
}

std::shared_ptr<ModList> OneSixInstance::loaderModList()
{
	I_D(OneSixInstance);
	if (!d->loader_mod_list)
	{
		d->loader_mod_list.reset(new ModList(loaderModsDir()));
	}
	else
		d->loader_mod_list->update();
	return d->loader_mod_list;
}

std::shared_ptr<ModList> OneSixInstance::resourcePackList()
{
	I_D(OneSixInstance);
	if (!d->resource_pack_list)
	{
		d->resource_pack_list.reset(new ModList(resourcePacksDir()));
	}
	else
		d->resource_pack_list->update();
	return d->resource_pack_list;
}

QDialog *OneSixInstance::createModEditDialog(QWidget *parent)
{
	return new OneSixModEditDialog(this, parent);
}

bool OneSixInstance::setIntendedVersionId(QString version)
{
	settings().set("IntendedVersion", version);
	setShouldUpdate(true);
	return true;
}

QString OneSixInstance::intendedVersionId() const
{
	return settings().get("IntendedVersion").toString();
}

void OneSixInstance::setShouldUpdate(bool val)
{
	settings().set("ShouldUpdate", val);
}

bool OneSixInstance::shouldUpdate() const
{
	I_D(OneSixInstance);
	QVariant var = settings().get("ShouldUpdate");
	if (!var.isValid() || var.toBool() == false)
	{
		return intendedVersionId() != currentVersionId();
	}
	return true;
}

bool OneSixInstance::versionIsCustom()
{
	QString verpath_custom = PathCombine(instanceRoot(), "custom.json");
	QFile versionfile(verpath_custom);
	return versionfile.exists();
}

QString OneSixInstance::currentVersionId() const
{
	return intendedVersionId();
}

bool OneSixInstance::customizeVersion()
{
	if (!versionIsCustom())
	{
		auto pathCustom = PathCombine(instanceRoot(), "custom.json");
		auto pathOrig = PathCombine(instanceRoot(), "version.json");
		QFile::copy(pathOrig, pathCustom);
		return reloadFullVersion();
	}
	else
		return true;
}

bool OneSixInstance::revertCustomVersion()
{
	if (versionIsCustom())
	{
		auto path = PathCombine(instanceRoot(), "custom.json");
		QFile::remove(path);
		return reloadFullVersion();
	}
	else
		return true;
}

bool OneSixInstance::reloadFullVersion()
{
	I_D(OneSixInstance);

	QString verpath = PathCombine(instanceRoot(), "version.json");
	{
		QString verpath_custom = PathCombine(instanceRoot(), "custom.json");
		QFile versionfile(verpath_custom);
		if (versionfile.exists())
			verpath = verpath_custom;
	}

	auto version = OneSixVersion::fromFile(verpath);
	if (version)
	{
		d->version = version;
		return true;
	}
	return false;
}

std::shared_ptr<OneSixVersion> OneSixInstance::getFullVersion()
{
	I_D(OneSixInstance);
	return d->version;
}

QString OneSixInstance::defaultBaseJar() const
{
	return "versions/" + intendedVersionId() + "/" + intendedVersionId() + ".jar";
}

QString OneSixInstance::defaultCustomBaseJar() const
{
	return PathCombine(instanceRoot(), "custom.jar");
}

bool OneSixInstance::menuActionEnabled(QString action_name) const
{
	if (action_name == "actionChangeInstLWJGLVersion")
		return false;
	return true;
}

QString OneSixInstance::getStatusbarDescription()
{
	QString descr = "One Six : " + intendedVersionId();
	if (versionIsCustom())
	{
		descr + " (custom)";
	}
	return descr;
}

QString OneSixInstance::loaderModsDir() const
{
	return PathCombine(minecraftRoot(), "mods");
}

QString OneSixInstance::resourcePacksDir() const
{
	return PathCombine(minecraftRoot(), "resourcepacks");
}

QString OneSixInstance::instanceConfigFolder() const
{
	return PathCombine(minecraftRoot(), "config");
}

