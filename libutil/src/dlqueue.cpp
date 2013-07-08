#include "include/dlqueue.h"

DownloadJob::DownloadJob ( QUrl url, QString target_path, QString expected_md5 )
	:Job()
{
	m_url = url;
	m_target_path = target_path;
	m_expected_md5 = expected_md5;

	m_check_md5 = m_expected_md5.size();
	m_save_to_file = m_target_path.size();
	m_status = Job_NotStarted;
}

JobPtr DownloadJob::create ( QUrl url, QString target_path, QString expected_md5 )
{
	return JobPtr ( new DownloadJob ( url, target_path, expected_md5 ) );
}

bool DownloadJob::ensurePathExists(QString filenamepath)
{
	QFileInfo a ( filenamepath );
	QDir dir;
	return (dir.mkpath ( a.path() ));
}

void DownloadJob::start()
{
	m_manager.reset ( new QNetworkAccessManager() );
	if ( m_save_to_file )
	{
		QString filename = m_target_path;
		m_output_file.setFileName ( filename );
		// if there already is a file and md5 checking is in effect and it can be opened
		if ( m_check_md5 && m_output_file.exists() && m_output_file.open ( QIODevice::ReadOnly ) )
		{
			// check the md5 against the expected one
			QString hash = QCryptographicHash::hash ( m_output_file.readAll(), QCryptographicHash::Md5 ).toHex().constData();
			m_output_file.close();
			// skip this file if they match
			if ( hash == m_expected_md5 )
			{
				qDebug() << "Skipping " << m_url.toString() << ": md5 match.";
				emit finish();
				return;
			}
		}
		if(!ensurePathExists(filename))
		{
			emit fail();
			return;
		}
		
		if ( !m_output_file.open ( QIODevice::WriteOnly ) )
		{
			/*
			 * Can't open the file... the job failed
			 */
			emit fail();
			return;
		}
	}
	qDebug() << "Downloading " << m_url.toString();
	QNetworkRequest request ( m_url );
	QNetworkReply * rep = m_manager->get ( request );
	m_reply = QSharedPointer<QNetworkReply> ( rep, &QObject::deleteLater );
	connect ( rep, SIGNAL ( downloadProgress ( qint64,qint64 ) ), SLOT ( downloadProgress ( qint64,qint64 ) ) );
	connect ( rep, SIGNAL ( finished() ), SLOT ( downloadFinished() ) );
	connect ( rep, SIGNAL ( error ( QNetworkReply::NetworkError ) ), SLOT ( downloadError ( QNetworkReply::NetworkError ) ) );
	connect ( rep, SIGNAL ( readyRead() ), SLOT ( downloadReadyRead() ) );
}

void DownloadJob::downloadProgress ( qint64 bytesReceived, qint64 bytesTotal )
{
	emit progress ( bytesReceived, bytesTotal );
}

void DownloadJob::downloadError ( QNetworkReply::NetworkError error )
{
	// error happened during download.
	// TODO: log the reason why
	m_status = Job_Failed;
}

void DownloadJob::downloadFinished()
{
	// if the download succeeded
	if ( m_status != Job_Failed )
	{
		// nothing went wrong...
		m_status = Job_Finished;
		// save the data to the downloadable if we aren't saving to file
		if ( !m_save_to_file )
		{
			m_data = m_reply->readAll();
		}
		else
		{
			m_output_file.close();
		}

		//TODO: check md5 here!
		m_reply.clear();
		emit finish();
		return;
	}
	// else the download failed
	else
	{
		if ( m_save_to_file )
		{
			m_output_file.close();
			m_output_file.remove();
		}
		m_reply.clear();
		emit fail();
		return;
	}
}

void DownloadJob::downloadReadyRead()
{
	if ( m_save_to_file )
	{
		m_output_file.write ( m_reply->readAll() );
	}
}
