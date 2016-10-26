
#include "InstanceImportTask.h"
#include "BaseInstance.h"
#include "BaseInstanceProvider.h"
#include "FileSystem.h"
#include "Env.h"
#include "MMCZip.h"
#include "NullInstance.h"
#include "settings/INISettingsObject.h"
#include "icons/IIconList.h"
#include <QtConcurrentRun>

InstanceImportTask::InstanceImportTask(SettingsObjectPtr settings, const QUrl sourceUrl, BaseInstanceProvider * target,
	const QString &instName, const QString &instIcon, const QString &instGroup)
{
	m_globalSettings = settings;
	m_sourceUrl = sourceUrl;
	m_target = target;
	m_instName = instName;
	m_instIcon = instIcon;
	m_instGroup = instGroup;
}

void InstanceImportTask::executeTask()
{
	InstancePtr newInstance;

	if (m_sourceUrl.isLocalFile())
	{
		m_archivePath = m_sourceUrl.toLocalFile();
		extractAndTweak();
	}
	else
	{
		setStatus(tr("Downloading modpack:\n%1").arg(m_sourceUrl.toString()));
		m_downloadRequired = true;

		const QString path = m_sourceUrl.host() + '/' + m_sourceUrl.path();
		auto entry = ENV.metacache()->resolveEntry("general", path);
		entry->setStale(true);
		m_filesNetJob.reset(new NetJob(tr("Modpack download")));
		m_filesNetJob->addNetAction(Net::Download::makeCached(m_sourceUrl, entry));
		m_archivePath = entry->getFullPath();
		auto job = m_filesNetJob.get();
		connect(job, &NetJob::succeeded, this, &InstanceImportTask::downloadSucceeded);
		connect(job, &NetJob::progress, this, &InstanceImportTask::downloadProgressChanged);
		connect(job, &NetJob::failed, this, &InstanceImportTask::downloadFailed);
		m_filesNetJob->start();
	}
}

void InstanceImportTask::downloadSucceeded()
{
	extractAndTweak();
}

void InstanceImportTask::downloadFailed(QString reason)
{
	emitFailed(reason);
}

void InstanceImportTask::downloadProgressChanged(qint64 current, qint64 total)
{
	setProgress(current / 2, total);
}

static QFileInfo findRecursive(const QString &dir, const QString &name)
{
	for (const auto info : QDir(dir).entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files, QDir::DirsLast))
	{
		if (info.isFile() && info.fileName() == name)
		{
			return info;
		}
		else if (info.isDir())
		{
			const QFileInfo res = findRecursive(info.absoluteFilePath(), name);
			if (res.isFile() && res.exists())
			{
				return res;
			}
		}
	}
	return QFileInfo();
}

void InstanceImportTask::extractAndTweak()
{
	setStatus(tr("Extracting modpack"));
	m_stagingPath = m_target->getStagedInstancePath();
	QDir extractDir(m_stagingPath);
	qDebug() << "Attempting to create instance from" << m_archivePath;

	m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractDir, m_archivePath, extractDir.absolutePath());
	connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &InstanceImportTask::extractFinished);
	connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &InstanceImportTask::extractAborted);
	m_extractFutureWatcher.setFuture(m_extractFuture);
}

void InstanceImportTask::extractFinished()
{
	if (m_extractFuture.result().isEmpty())
	{
		m_target->destroyStagingPath(m_stagingPath);
		emitFailed(tr("Failed to extract modpack"));
		return;
	}
	QDir extractDir(m_stagingPath);
	const QFileInfo instanceCfgFile = findRecursive(extractDir.absolutePath(), "instance.cfg");
	if (!instanceCfgFile.isFile() || !instanceCfgFile.exists())
	{
		m_target->destroyStagingPath(m_stagingPath);
		emitFailed(tr("Archive does not contain instance.cfg"));
		return;
	}

	// FIXME: copy from FolderInstanceProvider!!! FIX IT!!!
	auto instanceSettings = std::make_shared<INISettingsObject>(instanceCfgFile.absoluteFilePath());
	instanceSettings->registerSetting("InstanceType", "Legacy");

	QString actualDir = instanceCfgFile.absolutePath();
	NullInstance instance(m_globalSettings, instanceSettings, actualDir);

	// reset time played on import... because packs.
	instance.resetTimePlayed();

	// set a new nice name
	instance.setName(m_instName);

	// if the icon was specified by user, use that. otherwise pull icon from the pack
	if (m_instIcon != "default")
	{
		instance.setIconKey(m_instIcon);
	}
	else
	{
		m_instIcon = instance.iconKey();
		auto importIconPath = FS::PathCombine(instance.instanceRoot(), m_instIcon + ".png");
		if (QFile::exists(importIconPath))
		{
			// import icon
			auto iconList = ENV.icons();
			if (iconList->iconFileExists(m_instIcon))
			{
				iconList->deleteIcon(m_instIcon);
			}
			iconList->installIcons({importIconPath});
		}
	}
	if (!m_target->commitStagedInstance(m_stagingPath, actualDir, m_instName, m_instGroup))
	{
		m_target->destroyStagingPath(m_stagingPath);
		emitFailed(tr("Unable to commit instance"));
		return;
	}
	emitSucceeded();
}

void InstanceImportTask::extractAborted()
{
	m_target->destroyStagingPath(m_stagingPath);
	emitFailed(tr("Instance import has been aborted."));
	return;
}
