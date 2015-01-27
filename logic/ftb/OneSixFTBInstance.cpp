#include "OneSixFTBInstance.h"

#include "logic/minecraft/MinecraftProfile.h"
#include "logic/minecraft/OneSixLibrary.h"
#include "logic/minecraft/VersionBuilder.h"
#include "logic/tasks/SequentialTask.h"
#include "logic/forge/ForgeInstaller.h"
#include "logic/forge/ForgeVersionList.h"
#include "MultiMC.h"
#include "pathutils.h"

OneSixFTBInstance::OneSixFTBInstance(const QString &rootDir, SettingsObject *settings, QObject *parent) :
	OneSixInstance(rootDir, settings, parent)
{
}

void OneSixFTBInstance::copy(const QDir &newDir)
{
	QStringList libraryNames;
	// create patch file
	{
		QLOG_DEBUG() << "Creating patch file for FTB instance...";
		QFile f(minecraftRoot() + "/pack.json");
		if (!f.open(QFile::ReadOnly))
		{
			QLOG_ERROR() << "Couldn't open" << f.fileName() << ":" << f.errorString();
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
			QLOG_ERROR() << "Couldn't open" << out.fileName() << ":" << out.errorString();
			return;
		}
		out.write(QJsonDocument(root).toJson());
	}
	// copy libraries
	{
		QLOG_DEBUG() << "Copying FTB libraries";
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
				QLOG_ERROR() << "Couldn't create folder structure for" << out;
			}
			if (!QFile::copy(librariesPath().absoluteFilePath(lib->storagePath()), out))
			{
				QLOG_ERROR() << "Couldn't copy" << lib->rawName();
			}
		}
	}
}

QString OneSixFTBInstance::id() const
{
	return "FTB/" + BaseInstance::id();
}

QDir OneSixFTBInstance::librariesPath() const
{
	return QDir(MMC->settings()->get("FTBRoot").toString() + "/libraries");
}

QDir OneSixFTBInstance::versionsPath() const
{
	return QDir(MMC->settings()->get("FTBRoot").toString() + "/versions");
}

/*
QStringList OneSixFTBInstance::externalPatches() const
{
	return QStringList() << versionsPath().absoluteFilePath(intendedVersionId() + "/" + intendedVersionId() + ".json")
						 << minecraftRoot() + "/pack.json";
}
*/

bool OneSixFTBInstance::providesVersionFile() const
{
	return true;
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
