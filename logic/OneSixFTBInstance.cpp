#include "OneSixFTBInstance.h"

#include "OneSixVersion.h"
#include "OneSixLibrary.h"
#include "tasks/SequentialTask.h"
#include "ForgeInstaller.h"
#include "lists/ForgeVersionList.h"
#include "OneSixInstance_p.h"
#include "MultiMC.h"

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

QString OneSixFTBInstance::getStatusbarDescription()
{
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
