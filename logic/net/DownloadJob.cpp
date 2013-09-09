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
	parts_progress.append(QPair<qint64, qint64>(0,1));
	total_progress++;
	return ptr;
}

FileDownloadPtr DownloadJob::add ( QUrl url, QString rel_target_path)
{
	FileDownloadPtr ptr (new FileDownload(url, rel_target_path));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	parts_progress.append(QPair<qint64, qint64>(0,1));
	total_progress++;
	return ptr;
}

CacheDownloadPtr DownloadJob::add ( QUrl url, MetaEntryPtr entry)
{
	CacheDownloadPtr ptr (new CacheDownload(url, entry));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	parts_progress.append(QPair<qint64, qint64>(0,1));
	total_progress++;
	return ptr;
}

void DownloadJob::partSucceeded ( int index )
{
	// do progress. all slots are 1 in size at least
	auto & slot = parts_progress[index];
	partProgress ( index, slot.second , slot.second );
	
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
	auto & slot = parts_progress[index];
	
	current_progress -= slot.first;
	slot.first = bytesReceived;
	current_progress += slot.first;
	
	total_progress -= slot.second;
	slot.second = bytesTotal;
	total_progress += slot.second;
	emit progress(current_progress, total_progress);
}


void DownloadJob::start()
{
	qDebug() << m_job_name.toLocal8Bit() << " started.";
	for(auto iter: downloads)
	{
		connect(iter.data(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
		connect(iter.data(), SIGNAL(failed(int)), SLOT(partFailed(int)));
		connect(iter.data(), SIGNAL(progress(int,qint64,qint64)), SLOT(partProgress(int,qint64,qint64)));
		iter->start();
	}
}
