#include "MinecraftInstance.h"
#include <minecraft/launch/CreateServerResourcePacksFolder.h>
#include <minecraft/launch/ExtractNatives.h>
#include <minecraft/launch/PrintInstanceInfo.h>
#include <settings/Setting.h>
#include "settings/SettingsObject.h"
#include "Env.h"
#include <MMCStrings.h>
#include <pathmatcher/RegexpMatcher.h>
#include <pathmatcher/MultiMatcher.h>
#include <FileSystem.h>
#include <java/JavaVersion.h>

#include "launch/LaunchTask.h"
#include "launch/steps/PostLaunchCommand.h"
#include "launch/steps/Update.h"
#include "launch/steps/PreLaunchCommand.h"
#include "launch/steps/TextPrint.h"
#include "minecraft/launch/LauncherPartLaunch.h"
#include "minecraft/launch/DirectJavaLaunch.h"
#include "minecraft/launch/ModMinecraftJar.h"
#include "minecraft/launch/ClaimAccount.h"
#include "java/launch/CheckJava.h"
#include "java/JavaUtils.h"
#include "meta/Index.h"
#include "meta/VersionList.h"

#include "ModList.h"
#include "WorldList.h"

#include "icons/IIconList.h"

#include <QCoreApplication>
#include "MinecraftProfile.h"
#include "AssetsUtils.h"
#include "MinecraftUpdate.h"

#define IBUS "@im=ibus"

// all of this because keeping things compatible with deprecated old settings
// if either of the settings {a, b} is true, this also resolves to true
class OrSetting : public Setting
{
	Q_OBJECT
public:
	OrSetting(QString id, std::shared_ptr<Setting> a, std::shared_ptr<Setting> b)
	:Setting({id}, false), m_a(a), m_b(b)
	{
	}
	virtual QVariant get() const
	{
		bool a = m_a->get().toBool();
		bool b = m_b->get().toBool();
		return a || b;
	}
	virtual void reset() {}
	virtual void set(QVariant value) {}
private:
	std::shared_ptr<Setting> m_a;
	std::shared_ptr<Setting> m_b;
};

MinecraftInstance::MinecraftInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
	: BaseInstance(globalSettings, settings, rootDir)
{
	// FIXME: remove these
	m_settings->registerSetting({"IntendedVersion", "MinecraftVersion"}, "");
	m_settings->registerSetting("LWJGLVersion", "2.9.1");
	m_settings->registerSetting("ForgeVersion", "");
	m_settings->registerSetting("LiteloaderVersion", "");

	// Java Settings
	auto javaOverride = m_settings->registerSetting("OverrideJava", false);
	auto locationOverride = m_settings->registerSetting("OverrideJavaLocation", false);
	auto argsOverride = m_settings->registerSetting("OverrideJavaArgs", false);

	// combinations
	auto javaOrLocation = std::make_shared<OrSetting>("JavaOrLocationOverride", javaOverride, locationOverride);
	auto javaOrArgs = std::make_shared<OrSetting>("JavaOrArgsOverride", javaOverride, argsOverride);

	m_settings->registerOverride(globalSettings->getSetting("JavaPath"), javaOrLocation);
	m_settings->registerOverride(globalSettings->getSetting("JvmArgs"), javaOrArgs);

	// special!
	m_settings->registerPassthrough(globalSettings->getSetting("JavaTimestamp"), javaOrLocation);
	m_settings->registerPassthrough(globalSettings->getSetting("JavaVersion"), javaOrLocation);
	m_settings->registerPassthrough(globalSettings->getSetting("JavaArchitecture"), javaOrLocation);

	// Window Size
	auto windowSetting = m_settings->registerSetting("OverrideWindow", false);
	m_settings->registerOverride(globalSettings->getSetting("LaunchMaximized"), windowSetting);
	m_settings->registerOverride(globalSettings->getSetting("MinecraftWinWidth"), windowSetting);
	m_settings->registerOverride(globalSettings->getSetting("MinecraftWinHeight"), windowSetting);

	// Memory
	auto memorySetting = m_settings->registerSetting("OverrideMemory", false);
	m_settings->registerOverride(globalSettings->getSetting("MinMemAlloc"), memorySetting);
	m_settings->registerOverride(globalSettings->getSetting("MaxMemAlloc"), memorySetting);
	m_settings->registerOverride(globalSettings->getSetting("PermGen"), memorySetting);

	// Minecraft launch method
	auto launchMethodOverride = m_settings->registerSetting("OverrideMCLaunchMethod", false);
	m_settings->registerOverride(globalSettings->getSetting("MCLaunchMethod"), launchMethodOverride);
}

void MinecraftInstance::init()
{
	createProfile();
}

QString MinecraftInstance::typeName() const
{
	return "Minecraft";
}


bool MinecraftInstance::reload()
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

void MinecraftInstance::createProfile()
{
	m_profile.reset(new MinecraftProfile(this));
}

void MinecraftInstance::reloadProfile()
{
	m_profile->reload();
	setVersionBroken(m_profile->getProblemSeverity() == ProblemSeverity::Error);
	emit versionReloaded();
}

void MinecraftInstance::clearProfile()
{
	m_profile->clear();
	emit versionReloaded();
}

std::shared_ptr<MinecraftProfile> MinecraftInstance::getMinecraftProfile() const
{
	return m_profile;
}

QSet<QString> MinecraftInstance::traits() const
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

QString MinecraftInstance::minecraftRoot() const
{
	QFileInfo mcDir(FS::PathCombine(instanceRoot(), "minecraft"));
	QFileInfo dotMCDir(FS::PathCombine(instanceRoot(), ".minecraft"));

	if (mcDir.exists() && !dotMCDir.exists())
		return mcDir.filePath();
	else
		return dotMCDir.filePath();
}

QString MinecraftInstance::binRoot() const
{
	return FS::PathCombine(minecraftRoot(), "bin");
}

QString MinecraftInstance::getNativePath() const
{
	QDir natives_dir(FS::PathCombine(instanceRoot(), "natives/"));
	return natives_dir.absolutePath();
}

QString MinecraftInstance::getLocalLibraryPath() const
{
	QDir libraries_dir(FS::PathCombine(instanceRoot(), "libraries/"));
	return libraries_dir.absolutePath();
}

QString MinecraftInstance::loaderModsDir() const
{
	return FS::PathCombine(minecraftRoot(), "mods");
}

QString MinecraftInstance::coreModsDir() const
{
	return FS::PathCombine(minecraftRoot(), "coremods");
}

QString MinecraftInstance::resourcePacksDir() const
{
	return FS::PathCombine(minecraftRoot(), "resourcepacks");
}

QString MinecraftInstance::texturePacksDir() const
{
	return FS::PathCombine(minecraftRoot(), "texturepacks");
}

QString MinecraftInstance::instanceConfigFolder() const
{
	return FS::PathCombine(minecraftRoot(), "config");
}

QString MinecraftInstance::jarModsDir() const
{
	return FS::PathCombine(instanceRoot(), "jarmods");
}

QString MinecraftInstance::libDir() const
{
	return FS::PathCombine(minecraftRoot(), "lib");
}

QString MinecraftInstance::worldDir() const
{
	return FS::PathCombine(minecraftRoot(), "saves");
}

QDir MinecraftInstance::librariesPath() const
{
	return QDir::current().absoluteFilePath("libraries");
}

QDir MinecraftInstance::jarmodsPath() const
{
	return QDir(jarModsDir());
}

QDir MinecraftInstance::versionsPath() const
{
	return QDir::current().absoluteFilePath("versions");
}

QStringList MinecraftInstance::getClassPath() const
{
	QStringList jars, nativeJars;
	auto javaArchitecture = settings()->get("JavaArchitecture").toString();
	m_profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
	return jars;
}

QString MinecraftInstance::getMainClass() const
{
	return m_profile->getMainClass();
}

QStringList MinecraftInstance::getNativeJars() const
{
	QStringList jars, nativeJars;
	auto javaArchitecture = settings()->get("JavaArchitecture").toString();
	m_profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
	return nativeJars;
}

QStringList MinecraftInstance::extraArguments() const
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

QStringList MinecraftInstance::javaArguments() const
{
	QStringList args;

	// custom args go first. we want to override them if we have our own here.
	args.append(extraArguments());

	// OSX dock icon and name
#ifdef Q_OS_MAC
	args << "-Xdock:icon=icon.png";
	args << QString("-Xdock:name=\"%1\"").arg(windowTitle());
#endif

	// HACK: Stupid hack for Intel drivers. See: https://mojang.atlassian.net/browse/MCL-767
#ifdef Q_OS_WIN32
	args << QString("-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_"
					"minecraft.exe.heapdump");
#endif

	args << QString("-Xms%1m").arg(settings()->get("MinMemAlloc").toInt());
	args << QString("-Xmx%1m").arg(settings()->get("MaxMemAlloc").toInt());

	// No PermGen in newer java.
	JavaVersion javaVersion = getJavaVersion();
	if(javaVersion.requiresPermGen())
	{
		auto permgen = settings()->get("PermGen").toInt();
		if (permgen != 64)
		{
			args << QString("-XX:PermSize=%1m").arg(permgen);
		}
	}

	args << "-Duser.language=en";

	return args;
}

QMap<QString, QString> MinecraftInstance::getVariables() const
{
	QMap<QString, QString> out;
	out.insert("INST_NAME", name());
	out.insert("INST_ID", id());
	out.insert("INST_DIR", QDir(instanceRoot()).absolutePath());
	out.insert("INST_MC_DIR", QDir(minecraftRoot()).absolutePath());
	out.insert("INST_JAVA", settings()->get("JavaPath").toString());
	out.insert("INST_JAVA_ARGS", javaArguments().join(' '));
	return out;
}

QProcessEnvironment MinecraftInstance::createEnvironment()
{
	// prepare the process environment
	QProcessEnvironment env = CleanEnviroment();

	// export some infos
	auto variables = getVariables();
	for (auto it = variables.begin(); it != variables.end(); ++it)
	{
		env.insert(it.key(), it.value());
	}
	return env;
}

static QString replaceTokensIn(QString text, QMap<QString, QString> with)
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

QStringList MinecraftInstance::processMinecraftArgs(AuthSessionPtr session) const
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
	// FIXME: this is wrong and should be run as an async task
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

QString MinecraftInstance::createLaunchScript(AuthSessionPtr session)
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

QStringList MinecraftInstance::verboseDescription(AuthSessionPtr session)
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

QMap<QString, QString> MinecraftInstance::createCensorFilterFromSession(AuthSessionPtr session)
{
	if(!session)
	{
		return QMap<QString, QString>();
	}
	auto & sessionRef = *session.get();
	QMap<QString, QString> filter;
	auto addToFilter = [&filter](QString key, QString value)
	{
		if(key.trimmed().size())
		{
			filter[key] = value;
		}
	};
	if (sessionRef.session != "-")
	{
		addToFilter(sessionRef.session, tr("<SESSION ID>"));
	}
	addToFilter(sessionRef.access_token, tr("<ACCESS TOKEN>"));
	addToFilter(sessionRef.client_token, tr("<CLIENT TOKEN>"));
	addToFilter(sessionRef.uuid, tr("<PROFILE ID>"));
	addToFilter(sessionRef.player_name, tr("<PROFILE NAME>"));

	auto i = sessionRef.u.properties.begin();
	while (i != sessionRef.u.properties.end())
	{
		if(i.key() == "preferredLanguage")
		{
			++i;
			continue;
		}
		addToFilter(i.value(), "<" + i.key().toUpper() + ">");
		++i;
	}
	return filter;
}

MessageLevel::Enum MinecraftInstance::guessLevel(const QString &line, MessageLevel::Enum level)
{
	QRegularExpression re("\\[(?<timestamp>[0-9:]+)\\] \\[[^/]+/(?<level>[^\\]]+)\\]");
	auto match = re.match(line);
	if(match.hasMatch())
	{
		// New style logs from log4j
		QString timestamp = match.captured("timestamp");
		QString levelStr = match.captured("level");
		if(levelStr == "INFO")
			level = MessageLevel::Message;
		if(levelStr == "WARN")
			level = MessageLevel::Warning;
		if(levelStr == "ERROR")
			level = MessageLevel::Error;
		if(levelStr == "FATAL")
			level = MessageLevel::Fatal;
		if(levelStr == "TRACE" || levelStr == "DEBUG")
			level = MessageLevel::Debug;
	}
	else
	{
		// Old style forge logs
		if (line.contains("[INFO]") || line.contains("[CONFIG]") || line.contains("[FINE]") ||
			line.contains("[FINER]") || line.contains("[FINEST]"))
			level = MessageLevel::Message;
		if (line.contains("[SEVERE]") || line.contains("[STDERR]"))
			level = MessageLevel::Error;
		if (line.contains("[WARNING]"))
			level = MessageLevel::Warning;
		if (line.contains("[DEBUG]"))
			level = MessageLevel::Debug;
	}
	if (line.contains("overwriting existing"))
		return MessageLevel::Fatal;
	//NOTE: this diverges from the real regexp. no unicode, the first section is + instead of *
	static const QString javaSymbol = "([a-zA-Z_$][a-zA-Z\\d_$]*\\.)+[a-zA-Z_$][a-zA-Z\\d_$]*";
	if (line.contains("Exception in thread")
		|| line.contains(QRegularExpression("\\s+at " + javaSymbol))
		|| line.contains(QRegularExpression("Caused by: " + javaSymbol))
		|| line.contains(QRegularExpression("([a-zA-Z_$][a-zA-Z\\d_$]*\\.)+[a-zA-Z_$]?[a-zA-Z\\d_$]*(Exception|Error|Throwable)"))
		|| line.contains(QRegularExpression("... \\d+ more$"))
		)
		return MessageLevel::Error;
	return level;
}

IPathMatcher::Ptr MinecraftInstance::getLogFileMatcher()
{
	auto combined = std::make_shared<MultiMatcher>();
	combined->add(std::make_shared<RegexpMatcher>(".*\\.log(\\.[0-9]*)?(\\.gz)?$"));
	combined->add(std::make_shared<RegexpMatcher>("crash-.*\\.txt"));
	combined->add(std::make_shared<RegexpMatcher>("IDMap dump.*\\.txt$"));
	combined->add(std::make_shared<RegexpMatcher>("ModLoader\\.txt(\\..*)?$"));
	return combined;
}

QString MinecraftInstance::getLogFileRoot()
{
	return minecraftRoot();
}

QString MinecraftInstance::prettifyTimeDuration(int64_t duration)
{
	int seconds = (int) (duration % 60);
	duration /= 60;
	int minutes = (int) (duration % 60);
	duration /= 60;
	int hours = (int) (duration % 24);
	int days = (int) (duration / 24);
	if((hours == 0)&&(days == 0))
	{
		return tr("%1m %2s").arg(minutes).arg(seconds);
	}
	if (days == 0)
	{
		return tr("%1h %2m").arg(hours).arg(minutes);
	}
	return tr("%1d %2h %3m").arg(days).arg(hours).arg(minutes);
}

QString MinecraftInstance::getStatusbarDescription()
{
	QStringList traits;
	if (hasVersionBroken())
	{
		traits.append(tr("broken"));
	}

	QString description;
	description.append(tr("Minecraft %1 (%2)").arg(getComponentVersion("net.minecraft")).arg(typeName()));
	if(totalTimePlayed() > 0)
	{
		description.append(tr(", played for %1").arg(prettifyTimeDuration(totalTimePlayed())));
	}
	if(hasCrashed())
	{
		description.append(tr(", has crashed."));
	}
	return description;
}

shared_qobject_ptr<Task> MinecraftInstance::createUpdateTask()
{
	return shared_qobject_ptr<Task>(new OneSixUpdate(this));
}

std::shared_ptr<LaunchTask> MinecraftInstance::createLaunchTask(AuthSessionPtr session)
{
	auto process = LaunchTask::create(std::dynamic_pointer_cast<MinecraftInstance>(getSharedPtr()));
	auto pptr = process.get();

	ENV.icons()->saveIcon(iconKey(), FS::PathCombine(minecraftRoot(), "icon.png"), "PNG");

	// print a header
	{
		process->appendStep(std::make_shared<TextPrint>(pptr, "Minecraft folder is:\n" + minecraftRoot() + "\n\n", MessageLevel::MultiMC));
	}

	// check java
	{
		auto step = std::make_shared<CheckJava>(pptr);
		process->appendStep(step);
	}

	// check launch method
	QStringList validMethods = {"LauncherPart", "DirectJava"};
	QString method = launchMethod();
	if(!validMethods.contains(method))
	{
		process->appendStep(std::make_shared<TextPrint>(pptr, "Selected launch method \"" + method + "\" is not valid.\n", MessageLevel::Fatal));
		return process;
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
		process->appendStep(std::make_shared<ClaimAccount>(pptr, session));
		process->appendStep(std::make_shared<Update>(pptr));
	}

	// if there are any jar mods
	if(getJarMods().size())
	{
		auto step = std::make_shared<ModMinecraftJar>(pptr);
		process->appendStep(step);
	}

	// print some instance info here...
	{
		auto step = std::make_shared<PrintInstanceInfo>(pptr, session);
		process->appendStep(step);
	}

	// create the server-resource-packs folder (workaround for Minecraft bug MCL-3732)
	{
		auto step = std::make_shared<CreateServerResourcePacksFolder>(pptr);
		process->appendStep(step);
	}

	// extract native jars if needed
	{
		auto step = std::make_shared<ExtractNatives>(pptr);
		process->appendStep(step);
	}

	{
		// actually launch the game
		auto method = launchMethod();
		if(method == "LauncherPart")
		{
			auto step = std::make_shared<LauncherPartLaunch>(pptr);
			step->setWorkingDirectory(minecraftRoot());
			step->setAuthSession(session);
			process->appendStep(step);
		}
		else if (method == "DirectJava")
		{
			auto step = std::make_shared<DirectJavaLaunch>(pptr);
			step->setWorkingDirectory(minecraftRoot());
			step->setAuthSession(session);
			process->appendStep(step);
		}
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
	m_launchProcess = process;
	emit launchTaskChanged(m_launchProcess);
	return m_launchProcess;
}

QString MinecraftInstance::launchMethod()
{
	return m_settings->get("MCLaunchMethod").toString();
}

JavaVersion MinecraftInstance::getJavaVersion() const
{
	return JavaVersion(settings()->get("JavaVersion").toString());
}

bool MinecraftInstance::setComponentVersion(const QString& uid, const QString& version)
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

QString MinecraftInstance::getComponentVersion(const QString& uid) const
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

std::shared_ptr<ModList> MinecraftInstance::loaderModList() const
{
	if (!m_loader_mod_list)
	{
		m_loader_mod_list.reset(new ModList(loaderModsDir()));
	}
	m_loader_mod_list->update();
	return m_loader_mod_list;
}

std::shared_ptr<ModList> MinecraftInstance::coreModList() const
{
	if (!m_core_mod_list)
	{
		m_core_mod_list.reset(new ModList(coreModsDir()));
	}
	m_core_mod_list->update();
	return m_core_mod_list;
}

std::shared_ptr<ModList> MinecraftInstance::resourcePackList() const
{
	if (!m_resource_pack_list)
	{
		m_resource_pack_list.reset(new ModList(resourcePacksDir()));
	}
	m_resource_pack_list->update();
	return m_resource_pack_list;
}

std::shared_ptr<ModList> MinecraftInstance::texturePackList() const
{
	if (!m_texture_pack_list)
	{
		m_texture_pack_list.reset(new ModList(texturePacksDir()));
	}
	m_texture_pack_list->update();
	return m_texture_pack_list;
}

std::shared_ptr<WorldList> MinecraftInstance::worldList() const
{
	if (!m_world_list)
	{
		m_world_list.reset(new WorldList(worldDir()));
	}
	return m_world_list;
}

QList< Mod > MinecraftInstance::getJarMods() const
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


#include "MinecraftInstance.moc"
