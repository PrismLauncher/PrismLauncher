#include "FTBPlugin.h"
#include "FTBVersion.h"
#include "LegacyFTBInstance.h"
#include "OneSixFTBInstance.h"
#include <BaseInstance.h>
#include <icons/IconList.h>
#include <InstanceList.h>
#include <minecraft/MinecraftVersionList.h>
#include <settings/INISettingsObject.h>
#include <pathutils.h>
#include "QDebug"
#include <QXmlStreamReader>
#include <QRegularExpression>

struct FTBRecord
{
	QString dirName;
	QString name;
	QString logo;
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
	QDir dir = QDir(globalSettings->get("FTBLauncherDataRoot").toString());
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

InstancePtr loadInstance(SettingsObjectPtr globalSettings, const QString &instDir)
{
	auto m_settings = std::make_shared<INISettingsObject>(PathCombine(instDir, "instance.cfg"));

	InstancePtr inst;

	m_settings->registerSetting("InstanceType", "Legacy");

	QString inst_type = m_settings->get("InstanceType").toString();

	if (inst_type == "LegacyFTB")
	{
		inst.reset(new LegacyFTBInstance(globalSettings, m_settings, instDir));
	}
	else if (inst_type == "OneSixFTB")
	{
		inst.reset(new OneSixFTBInstance(globalSettings, m_settings, instDir));
	}
	inst->init();
	return inst;
}

InstancePtr createInstance(SettingsObjectPtr globalSettings, MinecraftVersionPtr version, const QString &instDir)
{
	QDir rootDir(instDir);

	InstancePtr inst;

	if (!version)
	{
		qCritical() << "Can't create instance for non-existing MC version";
		return nullptr;
	}

	qDebug() << instDir.toUtf8();
	if (!rootDir.exists() && !rootDir.mkpath("."))
	{
		qCritical() << "Can't create instance folder" << instDir;
		return nullptr;
	}

	auto m_settings = std::make_shared<INISettingsObject>(PathCombine(instDir, "instance.cfg"));
	m_settings->registerSetting("InstanceType", "Legacy");

	if (version->usesLegacyLauncher())
	{
		m_settings->set("InstanceType", "LegacyFTB");
		inst.reset(new LegacyFTBInstance(globalSettings, m_settings, instDir));
		inst->setIntendedVersionId(version->descriptor());
	}
	else
	{
		m_settings->set("InstanceType", "OneSixFTB");
		inst.reset(new OneSixFTBInstance(globalSettings, m_settings, instDir));
		inst->setIntendedVersionId(version->descriptor());
		inst->init();
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
		QString iconKey = record.logo;
		iconKey.remove(QRegularExpression("\\..*"));
		ENV.icons()->addIcon(iconKey, iconKey, PathCombine(record.templateDir, record.logo),
							  MMCIcon::Transient);

		if (!QFileInfo(PathCombine(record.instanceDir, "instance.cfg")).exists())
		{
			qDebug() << "Converting " << record.name << " as new.";
			auto mcVersion = std::dynamic_pointer_cast<MinecraftVersion>(ENV.getVersion("net.minecraft", record.mcVersion));
			if (!mcVersion)
			{
				qCritical() << "Can't load instance " << record.instanceDir
							<< " because minecraft version " << record.mcVersion
							<< " can't be resolved.";
				continue;
			}

			auto instPtr = createInstance(globalSettings, mcVersion, record.instanceDir);
			if (!instPtr)
			{
				continue;
			}

			instPtr->setGroupInitial("FTB");
			instPtr->setName(record.name);
			instPtr->setIconKey(iconKey);
			instPtr->setIntendedVersionId(record.mcVersion);
			instPtr->setNotes(record.description);
			if (!InstanceList::continueProcessInstance(instPtr, InstanceList::NoCreateError, record.instanceDir, groupMap))
				continue;
			tempList.append(InstancePtr(instPtr));
		}
		else
		{
			qDebug() << "Loading existing " << record.name;
			auto instPtr = loadInstance(globalSettings, record.instanceDir);
			if (!instPtr)
			{
				continue;
			}
			instPtr->setGroupInitial("FTB");
			instPtr->setName(record.name);
			instPtr->setIconKey(iconKey);
			if (instPtr->intendedVersionId() != record.mcVersion)
			{
				instPtr->setIntendedVersionId(record.mcVersion);
			}
			instPtr->setNotes(record.description);
			if (!InstanceList::continueProcessInstance(instPtr, InstanceList::NoCreateError, record.instanceDir, groupMap))
				continue;
			tempList.append(InstancePtr(instPtr));
		}
	}
}

#ifdef Q_OS_WIN32
#include <windows.h>
static const int APPDATA_BUFFER_SIZE = 1024;
#endif

void FTBPlugin::initialize(SettingsObjectPtr globalSettings)
{
	// FTB
	globalSettings->registerSetting("TrackFTBInstances", false);
	QString ftbDataDefault;
#ifdef Q_OS_LINUX
	QString ftbDefault = ftbDataDefault = QDir::home().absoluteFilePath(".ftblauncher");
#elif defined(Q_OS_WIN32)
	wchar_t buf[APPDATA_BUFFER_SIZE];
	wchar_t newBuf[APPDATA_BUFFER_SIZE];
	QString ftbDefault, newFtbDefault, oldFtbDefault;
	if (!GetEnvironmentVariableW(L"LOCALAPPDATA", newBuf, APPDATA_BUFFER_SIZE))
	{
		if(!GetEnvironmentVariableW(L"USERPROFILE", newBuf, APPDATA_BUFFER_SIZE))
		{
			qCritical() << "Your LOCALAPPDATA folder is missing! If you are on windows, this means your system is broken.";
		}
		else
		{
			auto userHome = QString::fromWCharArray(newBuf);
			auto localAppData = PathCombine(QString::fromWCharArray(newBuf), "Local Settings", "Application Data");
			newFtbDefault = QDir(localAppData).absoluteFilePath("ftblauncher");
		}
	}
	else
	{
		newFtbDefault = QDir(QString::fromWCharArray(newBuf)).absoluteFilePath("ftblauncher");
	}
	if (!GetEnvironmentVariableW(L"APPDATA", buf, APPDATA_BUFFER_SIZE))
	{
		qCritical() << "Your APPDATA folder is missing! If you are on windows, this means your "
					   "system is broken.";
	}
	else
	{
		oldFtbDefault = QDir(QString::fromWCharArray(buf)).absoluteFilePath("ftblauncher");
	}
	if (QFile::exists(QDir(newFtbDefault).absoluteFilePath("ftblaunch.cfg")))
	{
		qDebug() << "Old FTB setup";
		ftbDefault = ftbDataDefault = oldFtbDefault;
	}
	else
	{
		qDebug() << "New FTB setup";
		ftbDefault = oldFtbDefault;
		ftbDataDefault = newFtbDefault;
	}
#elif defined(Q_OS_MAC)
	QString ftbDefault = ftbDataDefault =
		PathCombine(QDir::homePath(), "Library/Application Support/ftblauncher");
#endif
	globalSettings->registerSetting("FTBLauncherDataRoot", ftbDataDefault);
	globalSettings->registerSetting("FTBLauncherRoot", ftbDefault);
	qDebug() << "FTB Launcher paths:" << globalSettings->get("FTBLauncherDataRoot").toString()
			 << "and" << globalSettings->get("FTBLauncherRoot").toString();

	globalSettings->registerSetting("FTBRoot");
	if (globalSettings->get("FTBRoot").isNull())
	{
		QString ftbRoot;
		QFile f(QDir(globalSettings->get("FTBLauncherRoot").toString())
					.absoluteFilePath("ftblaunch.cfg"));
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
