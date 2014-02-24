#include "OneSixFTBInstance.h"

#include "OneSixVersion.h"
#include "OneSixLibrary.h"
#include "tasks/SequentialTask.h"
#include "ForgeInstaller.h"
#include "lists/ForgeVersionList.h"
#include "OneSixInstance_p.h"
#include "OneSixVersionBuilder.h"
#include "MultiMC.h"
#include "pathutils.h"

class OneSixFTBInstanceForge : public Task
{
	Q_OBJECT
public:
	explicit OneSixFTBInstanceForge(const QString &version, OneSixFTBInstance *inst, QObject *parent = 0) :
		Task(parent), instance(inst), version("Forge " + version)
	{
	}

	void executeTask()
	{
		for (int i = 0; i < MMC->forgelist()->count(); ++i)
		{
			if (MMC->forgelist()->at(i)->name() == version)
			{
				forgeVersion = std::dynamic_pointer_cast<ForgeVersion>(MMC->forgelist()->at(i));
				break;
			}
		}
		if (!forgeVersion)
		{
			emitFailed(QString("Couldn't find forge version ") + version );
			return;
		}
		entry = MMC->metacache()->resolveEntry("minecraftforge", forgeVersion->filename);
		if (entry->stale)
		{
			setStatus(tr("Downloading Forge..."));
			fjob = new NetJob("Forge download");
			fjob->addNetAction(CacheDownload::make(forgeVersion->installer_url, entry));
			connect(fjob, &NetJob::failed, [this](){emitFailed(m_failReason);});
			connect(fjob, &NetJob::succeeded, this, &OneSixFTBInstanceForge::installForge);
			connect(fjob, &NetJob::progress, [this](qint64 c, qint64 total){ setProgress(100 * c / total); });
			fjob->start();
		}
		else
		{
			installForge();
		}
	}

private
slots:
	void installForge()
	{
		setStatus(tr("Installing Forge..."));
		QString forgePath = entry->getFullPath();
		ForgeInstaller forge(forgePath, forgeVersion->universal_url);
		if (!instance->reloadVersion())
		{
			emitFailed(tr("Couldn't load the version config"));
			return;
		}
		auto version = instance->getFullVersion();
		if (!forge.add(instance))
		{
			emitFailed(tr("Couldn't install Forge"));
			return;
		}
		emitSucceeded();
	}

private:
	OneSixFTBInstance *instance;
	QString version;
	ForgeVersionPtr forgeVersion;
	MetaEntryPtr entry;
	NetJob *fjob;
};

OneSixFTBInstance::OneSixFTBInstance(const QString &rootDir, SettingsObject *settings, QObject *parent) :
	OneSixInstance(rootDir, settings, parent)
{
}

void OneSixFTBInstance::init()
{
	reloadVersion();
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
	if (flags() & VersionBrokenFlag)
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
