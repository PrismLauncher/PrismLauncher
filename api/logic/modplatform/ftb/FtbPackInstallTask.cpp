#include "FtbPackInstallTask.h"
#include "Env.h"
#include "MMCZip.h"
#include "QtConcurrent"
#include "BaseInstance.h"
#include "FileSystem.h"
#include "settings/INISettingsObject.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/ComponentList.h"
#include "minecraft/GradleSpecifier.h"

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
	setStatus(tr("Downloading zip for %1").arg(toInstall.name));

	auto entry = ENV.metacache()->resolveEntry("general", "FTBPacks/" + toInstall.name);
	m_downloader->downloadSelected(entry);

	connect(m_downloader, &FtbPackDownloader::downloadSucceded, this, &FtbPackInstallTask::onDownloadSucceeded);
	connect(m_downloader, &FtbPackDownloader::downloadProgress, this, &FtbPackInstallTask::onDownloadProgress);
	connect(m_downloader, &FtbPackDownloader::downloadFailed, this,&FtbPackInstallTask::onDownloadFailed);
	progress(1, 4);
}

void FtbPackInstallTask::onDownloadSucceeded(QString archivePath){
	abortable = false;
	unzip(archivePath);
}

void FtbPackInstallTask::onDownloadFailed(QString reason) {
	emitFailed(reason);
}

void FtbPackInstallTask::onDownloadProgress(qint64 current, qint64 total){
	abortable = true;
	progress(current, total * 4);
	setStatus(tr("Downloading zip for %1 (%2\%)").arg(m_downloader->getSelectedPack().name).arg(current / 10));
}

void FtbPackInstallTask::unzip(QString archivePath) {
	progress(2, 4);
	setStatus(tr("Extracting modpack"));
	QDir extractDir(m_stagingPath);

	m_packZip.reset(new QuaZip(archivePath));
	if(!m_packZip->open(QuaZip::mdUnzip)) {
		emitFailed(tr("Failed to open modpack file %1!").arg(archivePath));
		return;
	}

	m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractDir, archivePath, extractDir.absolutePath() + "/unzip");
	connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &FtbPackInstallTask::onUnzipFinished);
	connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &FtbPackInstallTask::onUnzipCanceled);
	m_extractFutureWatcher.setFuture(m_extractFuture);
}

void FtbPackInstallTask::onUnzipFinished() {
	install();
}

void FtbPackInstallTask::onUnzipCanceled() {
	emitAborted();
}

void FtbPackInstallTask::install() {
	progress(3, 4);
	FtbModpack toInstall = m_downloader->getSelectedPack();
	setStatus(tr("Installing modpack"));
	QDir unzipMcDir(m_stagingPath + "/unzip/minecraft");
	if(unzipMcDir.exists()) {
		//ok, found minecraft dir, move contents to instance dir
		if(!QDir().rename(m_stagingPath + "/unzip/minecraft", m_stagingPath + "/.minecraft")) {
			emitFailed(tr("Failed to move unzipped minecraft!"));
			return;
		}
	}

	QString instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
	auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
	instanceSettings->registerSetting("InstanceType", "Legacy");
	instanceSettings->set("InstanceType", "OneSix");

	MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
	auto components = instance.getComponentList();
	components->buildingFromScratch();
	components->setComponentVersion("net.minecraft", toInstall.mcVersion, true);

	bool fallback = true;

	//handle different versions
	QFile packJson(m_stagingPath + "/.minecraft/pack.json");
	QDir jarmodDir = QDir(m_stagingPath + "/unzip/instMods");
	if(packJson.exists()) {
		packJson.open(QIODevice::ReadOnly | QIODevice::Text);
		QJsonDocument doc = QJsonDocument::fromJson(packJson.readAll());
		packJson.close();

		//we only care about the libs
		QJsonArray libs = doc.object().value("libraries").toArray();

		foreach (const QJsonValue &value, libs) {
			QString nameValue = value.toObject().value("name").toString();
			if(!nameValue.startsWith("net.minecraftforge")) {
				continue;
			}

			GradleSpecifier forgeVersion(nameValue);

			components->setComponentVersion("net.minecraftforge", forgeVersion.version().replace(toInstall.mcVersion, "").replace("-", ""));
			packJson.remove();
			fallback = false;
			break;
		}

	}

	if(jarmodDir.exists()) {
		qDebug() << "Found jarmods, installing...";

		QStringList jarmods;
		for (auto info: jarmodDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files))
		{
			qDebug() << "Jarmod:" << info.fileName();
			jarmods.push_back(info.absoluteFilePath());
		}

		components->installJarMods(jarmods);
		fallback = false;
	}

	//just nuke unzip directory, it s not needed anymore
	FS::deletePath(m_stagingPath + "/unzip");

	if(fallback) {
		//TODO: Some fallback mechanism... or just keep failing!
		emitFailed(tr("No installation method found!"));
		return;
	}

	components->saveNow();

	progress(4, 4);

	instance.init();
	instance.setName(m_instName);
	if(m_instIcon == "default") {
		m_instIcon = "ftb_logo";
	}
	instance.setIconKey(m_instIcon);
	instance.setGroupInitial(m_instGroup);
	instanceSettings->resumeSave();

	emitSucceeded();
}

bool FtbPackInstallTask::abort()
{
	if(abortable) {
		return m_downloader->getNetJob()->abort();
	}
	return false;
}
