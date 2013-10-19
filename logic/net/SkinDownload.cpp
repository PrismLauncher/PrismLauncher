#include "MultiMC.h"
#include "SkinDownload.h"
#include "DownloadJob.h"
#include <pathutils.h>

#include <QImage>
#include <QPainter>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDateTime>
#include <logger/QsLog.h>

SkinDownload::SkinDownload(QString name)
{
	m_name = name;
	m_entry = MMC->metacache()->resolveEntry("skins", name + ".png");
	m_entry->stale = true;
	m_url = QUrl("http://skins.minecraft.net/MinecraftSkins/" + name + ".png");
}

void SkinDownload::start()
{
	auto job = new DownloadJob("Player skin: " + m_name);

	job->addCacheDownload(m_url, m_entry);
	m_job.reset(job);

	connect(m_job.get(), SIGNAL(started()), SLOT(downloadStarted()));
	connect(m_job.get(), SIGNAL(progress(qint64, qint64)), SLOT(downloadProgress(qint64, qint64)));
	connect(m_job.get(), SIGNAL(succeeded()), SLOT(downloadSucceeded()));
	connect(m_job.get(), SIGNAL(failed()), SLOT(downloadFailed()));

	m_job->start();
}

void SkinDownload::downloadStarted()
{
	//QLOG_INFO() << "Started skin download for " << m_name << ".";

	emit started();
}

void SkinDownload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	emit progress(bytesReceived, bytesTotal);
}

void SkinDownload::downloadSucceeded()
{
	//QLOG_INFO() << "Got skin for " << m_name << ", cropping.";

	emit succeeded();
}

void SkinDownload::downloadFailed()
{
	//QLOG_ERROR() << "Failed to download skin for: " << m_name;

	emit failed();
}
