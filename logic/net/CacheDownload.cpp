#include "MultiMC.h"
#include "CacheDownload.h"
#include <pathutils.h>

#include <QCryptographicHash>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

CacheDownload::CacheDownload(QUrl url, MetaEntryPtr entry)
	: Download(), md5sum(QCryptographicHash::Md5)
{
	m_url = url;
	m_entry = entry;
	m_target_path = entry->getFullPath();
	m_status = Job_NotStarted;
	m_opened_for_saving = false;
}

void CacheDownload::start()
{
	if (!m_entry->stale)
	{
		emit succeeded(index_within_job);
		return;
	}
	m_output_file.setFileName(m_target_path);
	// if there already is a file and md5 checking is in effect and it can be opened
	if (!ensureFilePathExists(m_target_path))
	{
		emit failed(index_within_job);
		return;
	}
	qDebug() << "Downloading " << m_url.toString();
	QNetworkRequest request(m_url);
	request.setRawHeader(QString("If-None-Match").toLatin1(), m_entry->etag.toLatin1());

	auto worker = MMC->qnam();
	QNetworkReply *rep = worker->get(request);

	m_reply = QSharedPointer<QNetworkReply>(rep, &QObject::deleteLater);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
			SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}

void CacheDownload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	emit progress(index_within_job, bytesReceived, bytesTotal);
}

void CacheDownload::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	// TODO: log the reason why
	m_status = Job_Failed;
}
void CacheDownload::downloadFinished()
{
	// if the download succeeded
	if (m_status != Job_Failed)
	{

		// nothing went wrong...
		m_status = Job_Finished;
		if (m_opened_for_saving)
		{
			// save the data to the downloadable if we aren't saving to file
			m_output_file.close();
			m_entry->md5sum = md5sum.result().toHex().constData();
		}
		else
		{
			if (m_output_file.open(QIODevice::ReadOnly))
			{
				m_entry->md5sum =
					QCryptographicHash::hash(m_output_file.readAll(), QCryptographicHash::Md5)
						.toHex()
						.constData();
				m_output_file.close();
			}
		}
		QFileInfo output_file_info(m_target_path);

		m_entry->etag = m_reply->rawHeader("ETag").constData();
		m_entry->last_changed_timestamp =
			output_file_info.lastModified().toUTC().toMSecsSinceEpoch();
		m_entry->stale = false;
		MMC->metacache()->updateEntry(m_entry);

		m_reply.clear();
		emit succeeded(index_within_job);
		return;
	}
	// else the download failed
	else
	{
		m_output_file.close();
		m_output_file.remove();
		m_reply.clear();
		emit failed(index_within_job);
		return;
	}
}

void CacheDownload::downloadReadyRead()
{
	if (!m_opened_for_saving)
	{
		if (!m_output_file.open(QIODevice::WriteOnly))
		{
			/*
			* Can't open the file... the job failed
			*/
			m_reply->abort();
			emit failed(index_within_job);
			return;
		}
		m_opened_for_saving = true;
	}
	QByteArray ba = m_reply->readAll();
	md5sum.addData(ba);
	m_output_file.write(ba);
}
