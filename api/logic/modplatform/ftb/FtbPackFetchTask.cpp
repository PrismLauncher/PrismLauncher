#include "FtbPackFetchTask.h"
#include <QDomDocument>

FtbPackFetchTask::FtbPackFetchTask() {

}

FtbPackFetchTask::~FtbPackFetchTask() {

}

void FtbPackFetchTask::fetch() {
	NetJob *netJob = new NetJob("FtbModpackFetch");

	QUrl url = QUrl("https://ftb.cursecdn.com/FTB2/static/modpacks.xml");
	qDebug() << "Downloading version info from " << url.toString();

	netJob->addNetAction(downloadPtr = Net::Download::makeByteArray(url, &modpacksXmlFileData));

	QObject::connect(netJob, &NetJob::succeeded, this, &FtbPackFetchTask::fileDownloadFinished);
	QObject::connect(netJob, &NetJob::failed, this, &FtbPackFetchTask::fileDownloadFailed);

	jobPtr.reset(netJob);
	netJob->start();
}

void FtbPackFetchTask::fileDownloadFinished(){

	jobPtr.reset();

	QDomDocument doc;

	QString errorMsg = "Unknown error.";
	int errorLine = -1;
	int errorCol = -1;

	if(!doc.setContent(modpacksXmlFileData, false, &errorMsg, &errorLine, &errorCol)){
		auto fullErrMsg = QString("Failed to fetch modpack data: %s %d:%d!").arg(errorMsg, errorLine, errorCol);
		qWarning() << fullErrMsg;
		emit failed(fullErrMsg);
		modpacksXmlFileData.clear();
		return;
	}

	modpacksXmlFileData.clear();

	FtbModpackList modpackList;

	QDomNodeList nodes = doc.elementsByTagName("modpack");
	for(int i = 0; i < nodes.length(); i++) {
		QDomElement element = nodes.at(i).toElement();

		FtbModpack modpack;
		modpack.name = element.attribute("name");
		modpack.currentVersion = element.attribute("version");
		modpack.mcVersion = element.attribute("mcVersion");
		modpack.description = element.attribute("description");
		modpack.mods = element.attribute("mods");
		modpack.image = element.attribute("image");
		modpack.oldVersions = element.attribute("oldVersions").split(";");
		modpack.broken = false;
		modpack.bugged = false;

		//remove empty if the xml is bugged
		for(QString curr : modpack.oldVersions) {
			if(curr.isNull() || curr.isEmpty()) {
				modpack.oldVersions.removeAll(curr);
				modpack.bugged = true;
				qWarning() << "Removed some empty versions from" << modpack.name;
			}
		}

		if(modpack.oldVersions.size() < 1) {
			if(!modpack.currentVersion.isNull() && !modpack.currentVersion.isEmpty()) {
				modpack.oldVersions.append(modpack.currentVersion);
				qWarning() << "Added current version to oldVersions because oldVersions was empty! (" + modpack.name + ")";
			} else {
				modpack.broken = true;
				qWarning() << "Broken pack:" << modpack.name << " => No valid version!";
			}
		}

		modpack.author = element.attribute("author");

		modpack.dir = element.attribute("dir");
		modpack.file = element.attribute("url");

		modpackList.append(modpack);
	}


	emit finished(modpackList);
}

void FtbPackFetchTask::fileDownloadFailed(QString reason){
	qWarning() << "Fetching FtbPacks failed: " << reason;
	emit failed(reason);
}
