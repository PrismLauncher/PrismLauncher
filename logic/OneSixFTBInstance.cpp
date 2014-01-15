#include "OneSixFTBInstance.h"

#include "OneSixVersion.h"
#include "OneSixLibrary.h"
#include "tasks/SequentialTask.h"
#include "ForgeInstaller.h"
#include "lists/ForgeVersionList.h"
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
		if (!instance->reloadFullVersion())
		{
			emitFailed(tr("Couldn't load the version config"));
			return;
		}
		instance->revertCustomVersion();
		instance->customizeVersion();
		auto version = instance->getFullVersion();
		if (!forge.apply(version))
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
	QFile f(QDir(minecraftRoot()).absoluteFilePath("pack.json"));
	if (f.open(QFile::ReadOnly))
	{
		QString data = QString::fromUtf8(f.readAll());
		QRegularExpressionMatch match = QRegularExpression("net.minecraftforge:minecraftforge:[\\.\\d]*").match(data);
		m_forge.reset(new OneSixLibrary(match.captured()));
		m_forge->finalize();
	}
}

QString OneSixFTBInstance::id() const
{
	return "FTB/" + BaseInstance::id();
}

QString OneSixFTBInstance::getStatusbarDescription()
{
	return "OneSix FTB: " + intendedVersionId();
}
bool OneSixFTBInstance::menuActionEnabled(QString action_name) const
{
	return false;
}

std::shared_ptr<Task> OneSixFTBInstance::doUpdate(bool only_prepare)
{
	std::shared_ptr<SequentialTask> task;
	task.reset(new SequentialTask(this));
	if (!MMC->forgelist()->isLoaded())
	{
		task->addTask(std::shared_ptr<Task>(MMC->forgelist()->getLoadTask()));
	}
	task->addTask(OneSixInstance::doUpdate(only_prepare));
	task->addTask(std::shared_ptr<Task>(new OneSixFTBInstanceForge(m_forge->version(), this, this)));
	//FIXME: yes. this may appear dumb. but the previous step can change the list, so we do it all again.
	//TODO: Add a graph task. Construct graphs of tasks so we may capture the logic properly.
	task->addTask(OneSixInstance::doUpdate(only_prepare));
	return task;
}

#include "OneSixFTBInstance.moc"
