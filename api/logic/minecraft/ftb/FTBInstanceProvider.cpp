#include "FTBInstanceProvider.h"

#include <QDir>
#include <QDebug>
#include <QXmlStreamReader>
#include <QRegularExpression>

#include <settings/INISettingsObject.h>
#include <FileSystem.h>

#include "Env.h"

#include "LegacyFTBInstance.h"
#include "OneSixFTBInstance.h"

inline uint qHash(FTBRecord record)
{
	return qHash(record.instanceDir);
}

FTBInstanceProvider::FTBInstanceProvider(SettingsObjectPtr settings)
	: BaseInstanceProvider(settings)
{
	// nil
}

QList<InstanceId> FTBInstanceProvider::discoverInstances()
{
	// nothing to load when we don't have
	if (m_globalSettings->get("TrackFTBInstances").toBool() != true)
	{
		return {};
	}
	m_records.clear();
	discoverFTBEntries();
	return m_records.keys();
}

InstancePtr FTBInstanceProvider::loadInstance(const InstanceId& id)
{
	// process the records we acquired.
	auto iter = m_records.find(id);
	if(iter == m_records.end())
	{
		qWarning() << "Cannot load instance" << id << "without a record";
		return nullptr;
	}
	auto & record = m_records[id];
	qDebug() << "Loading FTB instance from " << record.instanceDir;
	QString iconKey = record.iconKey;
	auto icons = ENV.icons();
	if(icons)
	{
		icons->addIcon(iconKey, iconKey, FS::PathCombine(record.templateDir, record.logo), IconType::Transient);
	}
	auto settingsFilePath = FS::PathCombine(record.instanceDir, "instance.cfg");
	qDebug() << "ICON get!";

	if (QFileInfo(settingsFilePath).exists())
	{
		auto instPtr = loadInstance(record);
		if (!instPtr)
		{
			qWarning() << "Couldn't load instance config:" << settingsFilePath;
			if(!QFile::remove(settingsFilePath))
			{
				qWarning() << "Couldn't remove broken instance config!";
				return nullptr;
			}
			// failed to load, but removed the poisonous file
		}
		else
		{
			return InstancePtr(instPtr);
		}
	}
	auto instPtr = createInstance(record);
	if (!instPtr)
	{
		qWarning() << "Couldn't create FTB instance!";
		return nullptr;
	}
	return InstancePtr(instPtr);
}

void FTBInstanceProvider::discoverFTBEntries()
{
	QDir dir = QDir(m_globalSettings->get("FTBLauncherLocal").toString());
	QDir dataDir = QDir(m_globalSettings->get("FTBRoot").toString());
	if (!dataDir.exists())
	{
		qDebug() << "The FTB directory specified does not exist. Please check your settings";
		return;
	}
	else if (!dir.exists())
	{
		qDebug() << "The FTB launcher data directory specified does not exist. Please check "
					"your settings";
		return;
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
					auto id = "FTB/" + record.dirName;
					m_records[id] = record;
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
}

InstancePtr FTBInstanceProvider::loadInstance(const FTBRecord & record) const
{
	InstancePtr inst;

	auto m_settings = std::make_shared<INISettingsObject>(FS::PathCombine(record.instanceDir, "instance.cfg"));
	m_settings->registerSetting("InstanceType", "Legacy");

	qDebug() << "Loading existing " << record.name;

	QString inst_type = m_settings->get("InstanceType").toString();
	if (inst_type == "LegacyFTB")
	{
		inst.reset(new LegacyFTBInstance(m_globalSettings, m_settings, record.instanceDir));
	}
	else if (inst_type == "OneSixFTB")
	{
		inst.reset(new OneSixFTBInstance(m_globalSettings, m_settings, record.instanceDir));
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
	qDebug() << "Loaded instance " << inst->name() << " from " << inst->instanceRoot();
	return inst;
}

InstancePtr FTBInstanceProvider::createInstance(const FTBRecord & record) const
{
	QDir rootDir(record.instanceDir);

	InstancePtr inst;

	qDebug() << "Converting " << record.name << " as new.";

	if (!rootDir.exists() && !rootDir.mkpath("."))
	{
		qCritical() << "Can't create instance folder" << record.instanceDir;
		return nullptr;
	}

	auto m_settings = std::make_shared<INISettingsObject>(FS::PathCombine(record.instanceDir, "instance.cfg"));
	m_settings->registerSetting("InstanceType", "Legacy");

	// all legacy versions are built in. therefore we can do this even if we don't have ALL the versions Mojang has on their servers.
	m_settings->set("InstanceType", "OneSixFTB");
	inst.reset(new OneSixFTBInstance(m_globalSettings, m_settings, record.instanceDir));

	// initialize
	{
		SettingsObject::Lock lock(inst->settings());
		inst->setIntendedVersionId(record.mcVersion);
		inst->init();
		inst->setGroupInitial("FTB");
		inst->setName(record.name);
		inst->setIconKey(record.iconKey);
		inst->setNotes(record.description);
	}
	return inst;
}
