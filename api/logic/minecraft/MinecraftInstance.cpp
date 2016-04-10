#include "MinecraftInstance.h"
#include <settings/Setting.h>
#include "settings/SettingsObject.h"
#include "Env.h"
#include "minecraft/MinecraftVersionList.h"
#include <MMCStrings.h>
#include <pathmatcher/RegexpMatcher.h>
#include <pathmatcher/MultiMatcher.h>
#include <FileSystem.h>
#include <java/JavaVersion.h>

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
}

QString MinecraftInstance::minecraftRoot() const
{
	QFileInfo mcDir(FS::PathCombine(instanceRoot(), "minecraft"));
	QFileInfo dotMCDir(FS::PathCombine(instanceRoot(), ".minecraft"));

	if (dotMCDir.exists() && !mcDir.exists())
		return dotMCDir.filePath();
	else
		return mcDir.filePath();
}

std::shared_ptr< BaseVersionList > MinecraftInstance::versionList() const
{
	return ENV.getVersionList("net.minecraft");
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
	JavaVersion javaVersion(settings()->get("JavaVersion").toString());
	if(javaVersion.requiresPermGen())
	{
		auto permgen = settings()->get("PermGen").toInt();
		if (permgen != 64)
		{
			args << QString("-XX:PermSize=%1m").arg(permgen);
		}
	}

	args << "-Duser.language=en";
	args << "-jar" << FS::PathCombine(QCoreApplication::applicationDirPath(), "jars", "NewLaunch.jar");

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

static QString processLD_LIBRARY_PATH(const QString & LD_LIBRARY_PATH)
{
	QDir mmcBin(QCoreApplication::applicationDirPath());
	auto items = LD_LIBRARY_PATH.split(':');
	QStringList final;
	for(auto & item: items)
	{
		QDir test(item);
		if(test == mmcBin)
		{
			qDebug() << "Env:LD_LIBRARY_PATH ignoring path" << item;
			continue;
		}
		final.append(item);
	}
	return final.join(':');
}

QProcessEnvironment MinecraftInstance::createEnvironment()
{
	// prepare the process environment
	QProcessEnvironment rawenv = QProcessEnvironment::systemEnvironment();
	QProcessEnvironment env;

	QStringList ignored =
	{
		"JAVA_ARGS",
		"CLASSPATH",
		"CONFIGPATH",
		"JAVA_HOME",
		"JRE_HOME",
		"_JAVA_OPTIONS",
		"JAVA_OPTIONS",
		"JAVA_TOOL_OPTIONS"
	};
	for(auto key: rawenv.keys())
	{
		auto value = rawenv.value(key);
		// filter out dangerous java crap
		if(ignored.contains(key))
		{
			qDebug() << "Env: ignoring" << key << value;
			continue;
		}
		// filter MultiMC-related things
		if(key.startsWith("QT_"))
		{
			qDebug() << "Env: ignoring" << key << value;
			continue;
		}
#ifdef Q_OS_LINUX
		// Do not pass LD_* variables to java. They were intended for MultiMC
		if(key.startsWith("LD_"))
		{
			qDebug() << "Env: ignoring" << key << value;
			continue;
		}
		// Strip IBus
		// IBus is a Linux IME framework. For some reason, it breaks MC?
		if (key == "XMODIFIERS" && value.contains(IBUS))
		{
			QString save = value;
			value.replace(IBUS, "");
			qDebug() << "Env: stripped" << IBUS << "from" << save << ":" << value;
		}
		if(key == "GAME_PRELOAD")
		{
			env.insert("LD_PRELOAD", value);
			continue;
		}
		if(key == "GAME_LIBRARY_PATH")
		{
			env.insert("LD_LIBRARY_PATH", processLD_LIBRARY_PATH(value));
			continue;
		}
#endif
		qDebug() << "Env: " << key << value;
		env.insert(key, value);
	}
#ifdef Q_OS_LINUX
	// HACK: Workaround for QTBUG42500
	if(!env.contains("LD_LIBRARY_PATH"))
	{
		env.insert("LD_LIBRARY_PATH", "");
	}
#endif

	// export some infos
	auto variables = getVariables();
	for (auto it = variables.begin(); it != variables.end(); ++it)
	{
		env.insert(it.key(), it.value());
	}
	return env;
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
	if (flags() & VersionBrokenFlag)
	{
		traits.append(tr("broken"));
	}

	QString description;
	description.append(tr("Minecraft %1 (%2)").arg(intendedVersionId()).arg(typeName()));
	if(totalTimePlayed() > 0)
	{
		description.append(tr(", played for %1").arg(prettifyTimeDuration(totalTimePlayed())));
	}
	/*
	if(traits.size())
	{
		description.append(QString(" (%1)").arg(traits.join(", ")));
	}
	*/
	return description;
}

#include "MinecraftInstance.moc"
