#include "DownloadJob.h"
#include "pathutils.h"
#include "MultiMC.h"
#include "FileDownload.h"
#include "ByteArrayDownload.h"
#include "CacheDownload.h"

ByteArrayDownloadPtr DownloadJob::add ( QUrl url )
{
	ByteArrayDownloadPtr ptr (new ByteArrayDownload(url));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	return ptr;
}

FileDownloadPtr DownloadJob::add ( QUrl url, QString rel_target_path)
{
	FileDownloadPtr ptr (new FileDownload(url, rel_target_path));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	return ptr;
}

CacheDownloadPtr DownloadJob::add ( QUrl url, MetaEntryPtr entry)
{
	CacheDownloadPtr ptr (new CacheDownload(url, entry));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	return ptr;
}

void DownloadJob::partSucceeded ( int index )
{
	num_succeeded++;
	qDebug() << m_job_name.toLocal8Bit() << " progress: " << num_succeeded << "/" << downloads.size();
	if(num_failed + num_succeeded == downloads.size())
	{
		if(num_failed)
		{
			qDebug() << m_job_name.toLocal8Bit() << " failed.";
			emit failed();
		}
		else
		{
			qDebug() << m_job_name.toLocal8Bit() << " succeeded.";
			emit succeeded();
		}
	}
}

void DownloadJob::partFailed ( int index )
{
	num_failed++;
	if(num_failed + num_succeeded == downloads.size())
	{
		qDebug() << m_job_name.toLocal8Bit() << " failed.";
		emit failed();
	}
}

void DownloadJob::partProgress ( int index, qint64 bytesReceived, qint64 bytesTotal )
{
	// PROGRESS? DENIED!
}


void DownloadJob::start()
{
	qDebug() << m_job_name.toLocal8Bit() << " started.";
	for(auto iter: downloads)
	{
		connect(iter.data(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
		iter->start();
	}
}
