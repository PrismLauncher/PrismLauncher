#include "ByteArrayDownload.h"
#include "MultiMC.h"
#include <QDebug>

ByteArrayDownload::ByteArrayDownload(QUrl url) : Download()
{
	m_url = url;
	m_status = Job_NotStarted;
}

void ByteArrayDownload::start()
{
	qDebug() << "Downloading " << m_url.toString();
	QNetworkRequest request(m_url);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
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

void ByteArrayDownload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	emit progress(index_within_job, bytesReceived, bytesTotal);
}

void ByteArrayDownload::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	qDebug() << "URL:" << m_url.toString().toLocal8Bit() << "Network error: " << error;
	m_status = Job_Failed;
}

void ByteArrayDownload::downloadFinished()
{
	// if the download succeeded
	if (m_status != Job_Failed)
	{
		// nothing went wrong...
		m_status = Job_Finished;
		m_data = m_reply->readAll();
		m_reply.clear();
		emit succeeded(index_within_job);
		return;
	}
	// else the download failed
	else
	{
		m_reply.clear();
		emit failed(index_within_job);
		return;
	}
}

void ByteArrayDownload::downloadReadyRead()
{
	// ~_~
}
