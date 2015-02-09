#include "OneSixFTBInstance.h"
#include "FTBProfileStrategy.h"

#include "minecraft/MinecraftProfile.h"
#include "minecraft/OneSixLibrary.h"
#include "minecraft/VersionBuilder.h"
#include "tasks/SequentialTask.h"
#include "forge/ForgeInstaller.h"
#include "forge/ForgeVersionList.h"
#include <settings/INISettingsObject.h>
#include "pathutils.h"

OneSixFTBInstance::OneSixFTBInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir) :
	OneSixInstance(globalSettings, settings, rootDir)
{
	m_globalSettings = globalSettings;
}

void OneSixFTBInstance::copy(const QDir &newDir)
{
	QStringList libraryNames;
	// create patch file
	{
		qDebug()<< "Creating patch file for FTB instance...";
		QFile f(minecraftRoot() + "/pack.json");
		if (!f.open(QFile::ReadOnly))
		{
			qCritical() << "Couldn't open" << f.fileName() << ":" << f.errorString();
			return;
		}
		QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
		QJsonArray libs = root.value("libraries").toArray();
		QJsonArray outLibs;
		for (auto lib : libs)
		{
			QJsonObject libObj = lib.toObject();
			libObj.insert("MMC-hint", QString("local"));
			libObj.insert("insert", QString("prepend"));
			libraryNames.append(libObj.value("name").toString());
			outLibs.append(libObj);
		}
		root.remove("libraries");
		root.remove("id");

		// HACK HACK HACK HACK
		// A workaround for a problem in MultiMC, triggered by a historical problem in FTB,
		// triggered by Mojang getting their library versions wrong in 1.7.10
		if(intendedVersionId() == "1.7.10")
		{
			auto insert = [&outLibs, &libraryNames](QString name)
			{
				QJsonObject libObj;
				libObj.insert("insert", QString("replace"));
				libObj.insert("name", name);
				libraryNames.push_back(name);
				outLibs.prepend(libObj);
			};
			insert("com.google.guava:guava:16.0");
			insert("org.apache.commons:commons-lang3:3.2.1");
		}
		root.insert("+libraries", outLibs);
		root.insert("order", 1);
		root.insert("fileId", QString("org.multimc.ftb.pack.json"));
		root.insert("name", name());
		root.insert("mcVersion", intendedVersionId());
		root.insert("version", intendedVersionId());
		ensureFilePathExists(newDir.absoluteFilePath("patches/ftb.json"));
		QFile out(newDir.absoluteFilePath("patches/ftb.json"));
		if (!out.open(QFile::WriteOnly | QFile::Truncate))
		{
			qCritical() << "Couldn't open" << out.fileName() << ":" << out.errorString();
			return;
		}
		out.write(QJsonDocument(root).toJson());
	}
	// copy libraries
	{
		qDebug() << "Copying FTB libraries";
		for (auto library : libraryNames)
		{
			OneSixLibrary *lib = new OneSixLibrary(library);
			const QString out = QDir::current().absoluteFilePath("libraries/" + lib->storagePath());
			if (QFile::exists(out))
			{
				continue;
			}
			if (!ensureFilePathExists(out))
			{
				qCritical() << "Couldn't create folder structure for" << out;
			}
			if (!QFile::copy(librariesPath().absoluteFilePath(lib->storagePath()), out))
			{
				qCritical() << "Couldn't copy" << lib->rawName();
			}
		}
	}
	// now set the target instance to be plain OneSix
	INISettingsObject settings_obj(newDir.absoluteFilePath("instance.cfg"));
	settings_obj.registerSetting("InstanceType", "Legacy");
	QString inst_type = settings_obj.get("InstanceType").toString();
	settings_obj.set("InstanceType", "OneSix");
}

QString OneSixFTBInstance::id() const
{
	return "FTB/" + BaseInstance::id();
}

QDir OneSixFTBInstance::librariesPath() const
{
	return QDir(m_globalSettings->get("FTBRoot").toString() + "/libraries");
}

QDir OneSixFTBInstance::versionsPath() const
{
	return QDir(m_globalSettings->get("FTBRoot").toString() + "/versions");
}

bool OneSixFTBInstance::providesVersionFile() const
{
	return true;
}

void OneSixFTBInstance::createProfile()
{
	m_version.reset(new MinecraftProfile(new FTBProfileStrategy(this)));
}

QString OneSixFTBInstance::getStatusbarDescription()
{
	if (flags() & VersionBrokenFlag)
	{
		return "OneSix FTB: " + intendedVersionId() + " (broken)";
	}
	return "OneSix FTB: " + intendedVersionId();
}

std::shared_ptr<Task> OneSixFTBInstance::doUpdate()
{
	return OneSixInstance::doUpdate();
}

#include "OneSixFTBInstance.moc"
