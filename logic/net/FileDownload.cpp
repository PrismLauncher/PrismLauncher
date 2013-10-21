#include "MultiMC.h"
#include "FileDownload.h"
#include <pathutils.h>
#include <QCryptographicHash>
#include <logger/QsLog.h>


FileDownload::FileDownload ( QUrl url, QString target_path )
	:Download()
{
	m_url = url;
	m_target_path = target_path;
	m_check_md5 = false;
	m_status = Job_NotStarted;
	m_opened_for_saving = false;
}

void FileDownload::start()
{
	QString filename = m_target_path;
	m_output_file.setFileName ( filename );
	// if there already is a file and md5 checking is in effect and it can be opened
	if ( m_output_file.exists() && m_output_file.open ( QIODevice::ReadOnly ) )
	{
		// check the md5 against the expected one
		QString hash = QCryptographicHash::hash ( m_output_file.readAll(), QCryptographicHash::Md5 ).toHex().constData();
		m_output_file.close();
		// skip this file if they match
		if ( m_check_md5 && hash == m_expected_md5 )
		{
			QLOG_INFO() << "Skipping " << m_url.toString() << ": md5 match.";
			emit succeeded(index_within_job);
			return;
		}
		else
		{
			m_expected_md5 = hash;
		}
	}
	if(!ensureFilePathExists(filename))
	{
		emit failed(index_within_job);
		return;
	}
	
	QLOG_INFO() << "Downloading " << m_url.toString();
	QNetworkRequest request ( m_url );
	request.setRawHeader(QString("If-None-Match").toLatin1(), m_expected_md5.toLatin1()); 
	request.setHeader(QNetworkRequest::UserAgentHeader,"MultiMC/5.0 (Uncached)");
	
	auto worker = MMC->qnam();
	QNetworkReply * rep = worker->get ( request );
	
	m_reply = std::shared_ptr<QNetworkReply> ( rep );
	connect ( rep, SIGNAL ( downloadProgress ( qint64,qint64 ) ), SLOT ( downloadProgress ( qint64,qint64 ) ) );
	connect ( rep, SIGNAL ( finished() ), SLOT ( downloadFinished() ) );
	connect ( rep, SIGNAL ( error ( QNetworkReply::NetworkError ) ), SLOT ( downloadError ( QNetworkReply::NetworkError ) ) );
	connect ( rep, SIGNAL ( readyRead() ), SLOT ( downloadReadyRead() ) );
}

void FileDownload::downloadProgress ( qint64 bytesReceived, qint64 bytesTotal )
{
	emit progress (index_within_job, bytesReceived, bytesTotal );
}

void FileDownload::downloadError ( QNetworkReply::NetworkError error )
{
	// error happened during download.
	// TODO: log the reason why
	m_status = Job_Failed;
}

void FileDownload::downloadFinished()
{
	// if the download succeeded
	if ( m_status != Job_Failed )
	{
		// nothing went wrong...
		m_status = Job_Finished;
		m_output_file.close();

		m_reply.reset();
		emit succeeded(index_within_job);
		return;
	}
	// else the download failed
	else
	{
		m_output_file.close();
		m_reply.reset();
		emit failed(index_within_job);
		return;
	}
}

void FileDownload::downloadReadyRead()
{
	if(!m_opened_for_saving)
	{
		if ( !m_output_file.open ( QIODevice::WriteOnly ) )
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
	m_output_file.write ( m_reply->readAll() );
}
