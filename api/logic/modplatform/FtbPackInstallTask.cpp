#include "FtbPackInstallTask.h"
#include "Env.h"
#include "MMCZip.h"
#include "QtConcurrent"

FtbPackInstallTask::FtbPackInstallTask(FtbPackDownloader *downloader, SettingsObjectPtr settings,
						   const QString &stagingPath, const QString &instName, const QString &instIcon, const QString &instGroup) :
	m_globalSettings(settings), m_stagingPath(stagingPath), m_instName(instName), m_instIcon(instIcon), m_instGroup(instGroup)
{
	m_downloader = downloader;
}

void FtbPackInstallTask::executeTask() {
	downloadPack();
}

void FtbPackInstallTask::downloadPack(){
	FtbModpack toInstall = m_downloader->getSelectedPack();
	setStatus(tr("Installing new FTB Pack %1").arg(toInstall.name));

	auto entry = ENV.metacache()->resolveEntry("general", "FTBPack/" + toInstall.name);
	m_downloader->downloadSelected(entry);

	connect(m_downloader, &FtbPackDownloader::downloadSucceded, this, &FtbPackInstallTask::onDownloadSucceeded);
	connect(m_downloader, &FtbPackDownloader::downloadProgress, this, &FtbPackInstallTask::onDownloadProgress);
	connect(m_downloader, &FtbPackDownloader::downloadFailed, this,&FtbPackInstallTask::onDownloadFailed);
}

void FtbPackInstallTask::onDownloadSucceeded(QString archivePath){
	qDebug() << "Download succeeded!";
	unzip(archivePath);
}

void FtbPackInstallTask::onDownloadFailed(QString reason) {
	emitFailed(reason);
}

void FtbPackInstallTask::onDownloadProgress(qint64 current, qint64 total){
	progress(current, total);
}

void FtbPackInstallTask::unzip(QString archivePath) {
	setStatus(QString("Extracting modpack from %1").arg(archivePath));
	QDir extractDir(m_stagingPath);

	m_packZip.reset(new QuaZip(archivePath));
	if(!m_packZip->open(QuaZip::mdUnzip)) {
		emitFailed(tr("Failed to open modpack file %1!").arg(archivePath));
		return;
	}

	m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractSubDir, m_packZip.get(), QString("/"), extractDir.absolutePath());
	connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &FtbPackInstallTask::onUnzipFinished);
	connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &FtbPackInstallTask::onUnzipCanceled);
	m_extractFutureWatcher.setFuture(m_extractFuture);
}

void FtbPackInstallTask::onUnzipFinished() {
	qDebug() << "Unzipped:" << m_stagingPath;
	emitSucceeded();
}

void FtbPackInstallTask::onUnzipCanceled() {
	emitAborted();
}
