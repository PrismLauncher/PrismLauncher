#include "FTBPlugin.h"
#include <Env.h>
#include "FTBVersion.h"
#include "LegacyFTBInstance.h"
#include "OneSixFTBInstance.h"
#include <BaseInstance.h>
#include <InstanceList.h>
#include <minecraft/MinecraftVersionList.h>
#include <settings/INISettingsObject.h>
#include <FileSystem.h>
#include "QDebug"
#include <QXmlStreamReader>
#include <QRegularExpression>

struct FTBRecord
{
	QString dirName;
	QString name;
	QString logo;
	QString iconKey;
	QString mcVersion;
	QString description;
	QString instanceDir;
	QString templateDir;
	bool operator==(const FTBRecord other) const
	{
		return instanceDir == other.instanceDir;
	}
};

inline uint qHash(FTBRecord record)
{
	return qHash(record.instanceDir);
}

QSet<FTBRecord> discoverFTBInstances(SettingsObjectPtr globalSettings)
{
	QSet<FTBRecord> records;
	QDir dir = QDir(globalSettings->get("FTBLauncherLocal").toString());
	QDir dataDir = QDir(globalSettings->get("FTBRoot").toString());
	if (!dataDir.exists())
	{
		qDebug() << "The FTB directory specified does not exist. Please check your settings";
		return records;
	}
	else if (!dir.exists())
	{
		qDebug() << "The FTB launcher data directory specified does not exist. Please check "
					"your settings";
		return records;
	}
	dir.cd("ModPacks");
	auto allFiles = dir.entryList(QDir::Readable | QDir::Files, QDir::Name);
	for (auto filename : allFiles)
	{
		if (!filename.endsWith(".xml"))
			continue;
		auto fpath = dir.absoluteFilePath(filename);
		QFile f(fpath);
		qDebug() << "Discovering FTB instances -- " << fpath;
		if (!f.open(QFile::ReadOnly))
			continue;

		// read the FTB packs XML.
		QXmlStreamReader reader(&f);
		while (!reader.atEnd())
		{
			switch (reader.readNext())
			{
			case QXmlStreamReader::StartElement:
			{
				if (reader.name() == "modpack")
				{
					QXmlStreamAttributes attrs = reader.attributes();
					FTBRecord record;
					record.dirName = attrs.value("dir").toString();
					record.instanceDir = dataDir.absoluteFilePath(record.dirName);
					record.templateDir = dir.absoluteFilePath(record.dirName);
					QDir test(record.instanceDir);
					qDebug() << dataDir.absolutePath() << record.instanceDir << record.dirName;
					if (!test.exists())
						continue;
					record.name = attrs.value("name").toString();
					record.logo = attrs.value("logo").toString();
					QString logo = record.logo;
					record.iconKey = logo.remove(QRegularExpression("\\..*"));
					auto customVersions = attrs.value("customMCVersions");
					if (!customVersions.isNull())
					{
						QMap<QString, QString> versionMatcher;
						QString customVersionsStr = customVersions.toString();
						QStringList list = customVersionsStr.split(';');
						for (auto item : list)
						{
							auto segment = item.split('^');
							if (segment.size() != 2)
							{
								qCritical() << "FTB: Segment of size < 2 in "
											<< customVersionsStr;
								continue;
							}
							versionMatcher[segment[0]] = segment[1];
						}
						auto actualVersion = attrs.value("version").toString();
						if (versionMatcher.contains(actualVersion))
						{
							record.mcVersion = versionMatcher[actualVersion];
						}
						else
						{
							record.mcVersion = attrs.value("mcVersion").toString();
						}
					}
					else
					{
						record.mcVersion = attrs.value("mcVersion").toString();
					}
					record.description = attrs.value("description").toString();
					records.insert(record);
				}
				break;
			}
			case QXmlStreamReader::EndElement:
				break;
			case QXmlStreamReader::Characters:
				break;
			default:
				break;
			}
		}
		f.close();
	}
	return records;
}

InstancePtr loadInstance(SettingsObjectPtr globalSettings, QMap<QString, QString> &groupMap, const FTBRecord & record)
{
	InstancePtr inst;

	auto m_settings = std::make_shared<INISettingsObject>(FS::PathCombine(record.instanceDir, "instance.cfg"));
	m_settings->registerSetting("InstanceType", "Legacy");

	qDebug() << "Loading existing " << record.name;

	QString inst_type = m_settings->get("InstanceType").toString();
	if (inst_type == "LegacyFTB")
	{
		inst.reset(new LegacyFTBInstance(globalSettings, m_settings, record.instanceDir));
	}
	else if (inst_type == "OneSixFTB")
	{
		inst.reset(new OneSixFTBInstance(globalSettings, m_settings, record.instanceDir));
	}
	else
	{
		return nullptr;
	}
	qDebug() << "Construction " << record.instanceDir;

	SettingsObject::Lock lock(inst->settings());
	inst->init();
	qDebug() << "Init " << record.instanceDir;
	inst->setGroupInitial("FTB");
	/**
	 * FIXME: this does not respect the user's preferences. BUT, it would work nicely with the planned pack support
	 *        -> instead of changing the user values, change pack values (defaults you can look at and revert to)
	 */
	/*
	inst->setName(record.name);
	inst->setIconKey(record.iconKey);
	inst->setNotes(record.description);
	*/
	if (inst->intendedVersionId() != record.mcVersion)
	{
		inst->setIntendedVersionId(record.mcVersion);
	}
	qDebug() << "Post-Process " << record.instanceDir;
	if (!InstanceList::continueProcessInstance(inst, InstanceList::NoCreateError, record.instanceDir, groupMap))
	{
		return nullptr;
	}
	qDebug() << "Final " << record.instanceDir;
	return inst;
}

InstancePtr createInstance(SettingsObjectPtr globalSettings, QMap<QString, QString> &groupMap, const FTBRecord & record)
{
	QDir rootDir(record.instanceDir);

	InstancePtr inst;

	qDebug() << "Converting " << record.name << " as new.";

	auto mcVersion = std::dynamic_pointer_cast<MinecraftVersion>(ENV.getVersion("net.minecraft", record.mcVersion));
	if (!mcVersion)
	{
		qCritical() << "Can't load instance " << record.instanceDir
					<< " because minecraft version " << record.mcVersion
					<< " can't be resolved.";
		return nullptr;
	}

	if (!rootDir.exists() && !rootDir.mkpath("."))
	{
		qCritical() << "Can't create instance folder" << record.instanceDir;
		return nullptr;
	}

	auto m_settings = std::make_shared<INISettingsObject>(FS::PathCombine(record.instanceDir, "instance.cfg"));
	m_settings->registerSetting("InstanceType", "Legacy");

	if (mcVersion->usesLegacyLauncher())
	{
		m_settings->set("InstanceType", "LegacyFTB");
		inst.reset(new LegacyFTBInstance(globalSettings, m_settings, record.instanceDir));
	}
	else
	{
		m_settings->set("InstanceType", "OneSixFTB");
		inst.reset(new OneSixFTBInstance(globalSettings, m_settings, record.instanceDir));
	}
	// initialize
	{
		SettingsObject::Lock lock(inst->settings());
		inst->setIntendedVersionId(mcVersion->descriptor());
		inst->init();
		inst->setGroupInitial("FTB");
		inst->setName(record.name);
		inst->setIconKey(record.iconKey);
		inst->setNotes(record.description);
		qDebug() << "Post-Process " << record.instanceDir;
		if (!InstanceList::continueProcessInstance(inst, InstanceList::NoCreateError, record.instanceDir, groupMap))
		{
			return nullptr;
		}
	}
	return inst;
}

void FTBPlugin::loadInstances(SettingsObjectPtr globalSettings, QMap<QString, QString> &groupMap, QList<InstancePtr> &tempList)
{
	// nothing to load when we don't have
	if (globalSettings->get("TrackFTBInstances").toBool() != true)
	{
		return;
	}

	auto records = discoverFTBInstances(globalSettings);
	if (!records.size())
	{
		qDebug() << "No FTB instances to load.";
		return;
	}
	qDebug() << "Loading FTB instances! -- got " << records.size();
	// process the records we acquired.
	for (auto record : records)
	{
		qDebug() << "Loading FTB instance from " << record.instanceDir;
		QString iconKey = record.iconKey;
		// MMC->icons()->addIcon(iconKey, iconKey, FS::PathCombine(record.templateDir, record.logo), MMCIcon::Transient);
		auto settingsFilePath = FS::PathCombine(record.instanceDir, "instance.cfg");
		qDebug() << "ICON get!";

		if (QFileInfo(settingsFilePath).exists())
		{
			auto instPtr = loadInstance(globalSettings, groupMap, record);
			if (!instPtr)
			{
				qWarning() << "Couldn't load instance config:" << settingsFilePath;
				if(!QFile::remove(settingsFilePath))
				{
					qWarning() << "Couldn't remove broken instance config!";
					continue;
				}
				// failed to load, but removed the poisonous file
			}
			else
			{
				tempList.append(InstancePtr(instPtr));
				continue;
			}
		}
		auto instPtr = createInstance(globalSettings, groupMap, record);
		if (!instPtr)
		{
			qWarning() << "Couldn't create FTB instance!";
			continue;
		}
		tempList.append(InstancePtr(instPtr));
	}
}

#ifdef Q_OS_WIN32
#include <windows.h>
static const int APPDATA_BUFFER_SIZE = 1024;
#endif

static QString getLocalCacheStorageLocation()
{
	QString ftbDefault;
#ifdef Q_OS_WIN32
	wchar_t buf[APPDATA_BUFFER_SIZE];
	if (GetEnvironmentVariableW(L"LOCALAPPDATA", buf, APPDATA_BUFFER_SIZE)) // local
	{
		ftbDefault = QDir(QString::fromWCharArray(buf)).absoluteFilePath("ftblauncher");
	}
	else if (GetEnvironmentVariableW(L"APPDATA", buf, APPDATA_BUFFER_SIZE)) // roaming
	{
		ftbDefault = QDir(QString::fromWCharArray(buf)).absoluteFilePath("ftblauncher");
	}
	else
	{
		qCritical() << "Your LOCALAPPDATA and APPDATA folders are missing!"
			" If you are on windows, this means your system is broken.";
	}
#elif defined(Q_OS_MAC)
	ftbDefault = FS::PathCombine(QDir::homePath(), "Library/Application Support/ftblauncher");
#else
	ftbDefault = QDir::home().absoluteFilePath(".ftblauncher");
#endif
	return ftbDefault;
}


static QString getRoamingStorageLocation()
{
	QString ftbDefault;
#ifdef Q_OS_WIN32
	wchar_t buf[APPDATA_BUFFER_SIZE];
	QString cacheStorage;
	if (GetEnvironmentVariableW(L"APPDATA", buf, APPDATA_BUFFER_SIZE))
	{
		ftbDefault = QDir(QString::fromWCharArray(buf)).absoluteFilePath("ftblauncher");
	}
	else
	{
		qCritical() << "Your APPDATA folder is missing! If you are on windows, this means your system is broken.";
	}
#elif defined(Q_OS_MAC)
	ftbDefault = FS::PathCombine(QDir::homePath(), "Library/Application Support/ftblauncher");
#else
	ftbDefault = QDir::home().absoluteFilePath(".ftblauncher");
#endif
	return ftbDefault;
}

void FTBPlugin::initialize(SettingsObjectPtr globalSettings)
{
	// FTB
	globalSettings->registerSetting("TrackFTBInstances", false);
	QString ftbRoaming = getRoamingStorageLocation();
	QString ftbLocal = getLocalCacheStorageLocation();

	globalSettings->registerSetting("FTBLauncherRoaming", ftbRoaming);
	globalSettings->registerSetting("FTBLauncherLocal", ftbLocal);
	qDebug() << "FTB Launcher paths:" << globalSettings->get("FTBLauncherRoaming").toString()
			 << "and" << globalSettings->get("FTBLauncherLocal").toString();

	globalSettings->registerSetting("FTBRoot");
	if (globalSettings->get("FTBRoot").isNull())
	{
		QString ftbRoot;
		QFile f(QDir(globalSettings->get("FTBLauncherRoaming").toString()).absoluteFilePath("ftblaunch.cfg"));
		qDebug() << "Attempting to read" << f.fileName();
		if (f.open(QFile::ReadOnly))
		{
			const QString data = QString::fromLatin1(f.readAll());
			QRegularExpression exp("installPath=(.*)");
			ftbRoot = QDir::cleanPath(exp.match(data).captured(1));
#ifdef Q_OS_WIN32
			if (!ftbRoot.isEmpty())
			{
				if (ftbRoot.at(0).isLetter() && ftbRoot.size() > 1 && ftbRoot.at(1) == '/')
				{
					ftbRoot.remove(1, 1);
				}
			}
#endif
			if (ftbRoot.isEmpty())
			{
				qDebug() << "Failed to get FTB root path";
			}
			else
			{
				qDebug() << "FTB is installed at" << ftbRoot;
				globalSettings->set("FTBRoot", ftbRoot);
			}
		}
		else
		{
			qWarning() << "Couldn't open" << f.fileName() << ":" << f.errorString();
			qWarning() << "This is perfectly normal if you don't have FTB installed";
		}
	}
}
