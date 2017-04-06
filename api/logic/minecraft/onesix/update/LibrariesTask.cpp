#include "Env.h"
#include "LibrariesTask.h"
#include "minecraft/onesix/OneSixInstance.h"

LibrariesTask::LibrariesTask(OneSixInstance * inst)
{
	m_inst = inst;
}

void LibrariesTask::executeTask()
{
	setStatus(tr("Getting the library files from Mojang..."));
	qDebug() << m_inst->name() << ": downloading libraries";
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	inst->reloadProfile();
	if(inst->hasVersionBroken())
	{
		emitFailed(tr("Failed to load the version description files - check the instance for errors."));
		return;
	}

	// Build a list of URLs that will need to be downloaded.
	std::shared_ptr<MinecraftProfile> profile = inst->getMinecraftProfile();
	// minecraft.jar for this version
	{
		QString version_id = profile->getMinecraftVersion();
		QString localPath = version_id + "/" + version_id + ".jar";
		QString urlstr = profile->getMainJarUrl();

		auto job = new NetJob(tr("Libraries for instance %1").arg(inst->name()));

		auto metacache = ENV.metacache();
		auto entry = metacache->resolveEntry("versions", localPath);
		job->addNetAction(Net::Download::makeCached(QUrl(urlstr), entry));
		downloadJob.reset(job);
	}

	auto metacache = ENV.metacache();
	QList<LibraryPtr> brokenLocalLibs;
	QStringList failedFiles;
	auto createJobs = [&](const QList<LibraryPtr> & libs)
	{
		for (auto lib : libs)
		{
			auto dls = lib->getDownloads(currentSystem, metacache.get(), failedFiles, inst->getLocalLibraryPath());
			for(auto dl : dls)
			{
				downloadJob->addNetAction(dl);
			}
		}
	};
	createJobs(profile->getLibraries());
	createJobs(profile->getNativeLibraries());

	// FIXME: this is never filled!!!!
	if (!brokenLocalLibs.empty())
	{
		downloadJob.reset();
		QString failed_all = failedFiles.join("\n");
		emitFailed(tr("Some libraries marked as 'local' are missing their jar "
					"files:\n%1\n\nYou'll have to correct this problem manually. If this is "
					"an externally tracked instance, make sure to run it at least once "
					"outside of MultiMC.").arg(failed_all));
		return;
	}
	connect(downloadJob.get(), &NetJob::succeeded, this, &LibrariesTask::emitSucceeded);
	connect(downloadJob.get(), &NetJob::failed, this, &LibrariesTask::jarlibFailed);
	connect(downloadJob.get(), &NetJob::progress, this, &LibrariesTask::progress);
	downloadJob->start();
}

bool LibrariesTask::canAbort() const
{
	return true;
}

void LibrariesTask::jarlibFailed(QString reason)
{
	emitFailed(tr("Game update failed: it was impossible to fetch the required libraries.\nReason:\n%1").arg(reason));
}

bool LibrariesTask::abort()
{
	if(downloadJob)
	{
		return downloadJob->abort();
	}
	else
	{
		qWarning() << "Prematurely aborted LibrariesTask";
	}
	return true;
}
