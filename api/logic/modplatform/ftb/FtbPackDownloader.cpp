#include "FtbPackDownloader.h"
#include "PackHelpers.h"
#include "FtbPackFetchTask.h"
#include "Env.h"

FtbPackDownloader::FtbPackDownloader()
{
	done = false;
	fetching = false;
}

FtbPackDownloader::~FtbPackDownloader()
{
}

FtbModpackList FtbPackDownloader::getModpacks()
{
	return static_cast<FtbModpackList>(fetchedPacks.values());
}

void FtbPackDownloader::fetchModpacks(bool force = false)
{
	if(fetching || (!force && done))
	{
		qDebug() << "Skipping modpack refetch because done or already fetching [done =>" << done << "| fetching =>" << fetching << "]";
		return;
	}

	fetching = true;

	fetchTask = new FtbPackFetchTask();
	connect(fetchTask, &FtbPackFetchTask::finished, this, &FtbPackDownloader::fetchSuccess);
	connect(fetchTask, &FtbPackFetchTask::failed, this, &FtbPackDownloader::fetchFailed);
	fetchTask->fetch();
}


void FtbPackDownloader::fetchSuccess(FtbModpackList modpacks)
{
	for(int i = 0; i < modpacks.size(); i++)
	{
		fetchedPacks.insert(modpacks.at(i).name, modpacks.at(i));
	}

	fetching = false;
	done = true;
	emit ready();
	fetchTask->deleteLater();
}

void FtbPackDownloader::fetchFailed(QString reason)
{
	qWarning() << "Failed to fetch FtbData" << reason;
	fetching = false;
	emit packFetchFailed();
	fetchTask->deleteLater();
}
