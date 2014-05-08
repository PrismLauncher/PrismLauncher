#include "OneSixFTBInstance.h"

#include "logic/minecraft/VersionFinal.h"
#include "logic/minecraft/OneSixLibrary.h"
#include "logic/minecraft/OneSixVersionBuilder.h"
#include "tasks/SequentialTask.h"
#include "forge/ForgeInstaller.h"
#include "forge/ForgeVersionList.h"
#include "OneSixInstance_p.h"
#include "MultiMC.h"
#include "pathutils.h"

OneSixFTBInstance::OneSixFTBInstance(const QString &rootDir, SettingsObject *settings, QObject *parent) :
	OneSixInstance(rootDir, settings, parent)
{
}

void OneSixFTBInstance::init()
{
	try
	{
		reloadVersion();
	}
	catch(MMCError & e)
	{
		// QLOG_ERROR() << "Caught exception on instance init: " << e.cause();
	}
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
			lib->finalize();
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

QStringList OneSixFTBInstance::externalPatches() const
{
	I_D(OneSixInstance);
	return QStringList() << versionsPath().absoluteFilePath(intendedVersionId() + "/" + intendedVersionId() + ".json")
						 << minecraftRoot() + "/pack.json";
}

bool OneSixFTBInstance::providesVersionFile() const
{
	return true;
}

QString OneSixFTBInstance::getStatusbarDescription()
{
	if (flags().contains(VersionBrokenFlag))
	{
		return "OneSix FTB: " + intendedVersionId() + " (broken)";
	}
	return "OneSix FTB: " + intendedVersionId();
}
bool OneSixFTBInstance::menuActionEnabled(QString action_name) const
{
	return false;
}

std::shared_ptr<Task> OneSixFTBInstance::doUpdate()
{
	return OneSixInstance::doUpdate();
}

#include "OneSixFTBInstance.moc"
