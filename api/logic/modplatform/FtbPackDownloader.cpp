#include "FtbPackDownloader.h"
#include "PackHelpers.h"
#include "FtbPackFetchTask.h"
#include "Env.h"

FtbPackDownloader::FtbPackDownloader() {
	done = false;
	fetching = false;
}

FtbPackDownloader::~FtbPackDownloader(){
	delete netJobContainer.get();
	netJobContainer.reset(nullptr);
}

bool FtbPackDownloader::isValidPackSelected(){
	FtbModpack dummy;
	dummy.name = "__INVALID__";

	FtbModpack other = fetchedPacks.value(selected.name, dummy);
	if(other.name == "__INVALID__") {
		return false;
	}

	return other.oldVersions.contains(selectedVersion);
}

QString FtbPackDownloader::getSuggestedInstanceName() {
	return selected.name;
}

FtbModpackList FtbPackDownloader::getModpacks() {
	return static_cast<FtbModpackList>(fetchedPacks.values());
}

void FtbPackDownloader::fetchModpacks(bool force = false){
	if(fetching || (!force && done)) {
		qDebug() << "Skipping modpack refetch because done or already fetching [done =>" << done << "| fetching =>" << fetching << "]";
		return;
	}

	fetching = true;

	fetchTask = new FtbPackFetchTask();
	connect(fetchTask, &FtbPackFetchTask::finished, this, &FtbPackDownloader::fetchSuccess);
	connect(fetchTask, &FtbPackFetchTask::failed, this, &FtbPackDownloader::fetchFailed);
	fetchTask->fetch();
}


void FtbPackDownloader::fetchSuccess(FtbModpackList modpacks) {
	for(int i = 0; i < modpacks.size(); i++) {
		fetchedPacks.insert(modpacks.at(i).name, modpacks.at(i));
	}

	fetching = false;
	done = true;
	emit ready();
	fetchTask->deleteLater();
}

void FtbPackDownloader::fetchFailed(QString reason) {
	qWarning() << "Failed to fetch FtbData" << reason;
	fetching = false;
	emit packFetchFailed();
	fetchTask->deleteLater();
}

void FtbPackDownloader::selectPack(FtbModpack modpack, QString version) {
	selected = modpack;
	selectedVersion = version;
}

FtbModpack FtbPackDownloader::getSelectedPack() {
	return selected;
}

void FtbPackDownloader::downloadSelected(MetaEntryPtr cache) {
	NetJob *job = new NetJob("Downlad FTB Pack");

	cache->setStale(true);
	QString url = QString("http://ftb.cursecdn.com/FTB2/modpacks/%1/%2/%3").arg(selected.dir, selectedVersion.replace(".", "_"), selected.file);
	job->addNetAction(Net::Download::makeCached(url, cache));
	downloadPath = cache->getFullPath();

	netJobContainer.reset(job);

	connect(job, &NetJob::succeeded, this, &FtbPackDownloader::_downloadSucceeded);
	connect(job, &NetJob::failed, this, &FtbPackDownloader::_downloadFailed);
	connect(job, &NetJob::progress, this, &FtbPackDownloader::_downloadProgress);
	job->start();
}

void FtbPackDownloader::_downloadSucceeded() {
	netJobContainer.reset();
	emit downloadSucceded(downloadPath);
}

void FtbPackDownloader::_downloadProgress(qint64 current, qint64 total) {
	emit downloadProgress(current, total);
}

void FtbPackDownloader::_downloadFailed(QString reason) {
	netJobContainer.reset();
	emit downloadFailed(reason);
}
