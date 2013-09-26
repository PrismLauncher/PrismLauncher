#include "DownloadJob.h"
#include "pathutils.h"
#include "MultiMC.h"
#include "FileDownload.h"
#include "ByteArrayDownload.h"
#include "CacheDownload.h"

#include <QDebug>

ByteArrayDownloadPtr DownloadJob::add(QUrl url)
{
	ByteArrayDownloadPtr ptr(new ByteArrayDownload(url));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	parts_progress.append(part_info());
	total_progress++;
	return ptr;
}

FileDownloadPtr DownloadJob::add(QUrl url, QString rel_target_path)
{
	FileDownloadPtr ptr(new FileDownload(url, rel_target_path));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	parts_progress.append(part_info());
	total_progress++;
	return ptr;
}

CacheDownloadPtr DownloadJob::add(QUrl url, MetaEntryPtr entry)
{
	CacheDownloadPtr ptr(new CacheDownload(url, entry));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	parts_progress.append(part_info());
	total_progress++;
	return ptr;
}

void DownloadJob::partSucceeded(int index)
{
	// do progress. all slots are 1 in size at least
	auto &slot = parts_progress[index];
	partProgress(index, slot.total_progress, slot.total_progress);

	num_succeeded++;
	qDebug() << m_job_name.toLocal8Bit() << " progress: " << num_succeeded << "/"
			 << downloads.size();
	if (num_failed + num_succeeded == downloads.size())
	{
		if (num_failed)
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

void DownloadJob::partFailed(int index)
{
	auto &slot = parts_progress[index];
	if (slot.failures == 3)
	{
		qDebug() << "Part " << index << " failed 3 times (" << downloads[index]->m_url << ")";
		num_failed++;
		if (num_failed + num_succeeded == downloads.size())
		{
			qDebug() << m_job_name.toLocal8Bit() << " failed.";
			emit failed();
		}
	}
	else
	{
		qDebug() << "Part " << index << " failed, restarting (" << downloads[index]->m_url
				 << ")";
		// restart the job
		slot.failures++;
		downloads[index]->start();
	}
}

void DownloadJob::partProgress(int index, qint64 bytesReceived, qint64 bytesTotal)
{
	auto &slot = parts_progress[index];

	current_progress -= slot.current_progress;
	slot.current_progress = bytesReceived;
	current_progress += slot.current_progress;

	total_progress -= slot.total_progress;
	slot.total_progress = bytesTotal;
	total_progress += slot.total_progress;
	emit progress(current_progress, total_progress);
}

void DownloadJob::start()
{
	qDebug() << m_job_name.toLocal8Bit() << " started.";
	for (auto iter : downloads)
	{
		connect(iter.data(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
		connect(iter.data(), SIGNAL(failed(int)), SLOT(partFailed(int)));
		connect(iter.data(), SIGNAL(progress(int, qint64, qint64)),
				SLOT(partProgress(int, qint64, qint64)));
		iter->start();
	}
}

QStringList DownloadJob::getFailedFiles()
{
	QStringList failed;
	for (auto download : downloads)
	{
		if (download->m_status == Job_Failed)
		{
			failed.push_back(download->m_url.toString());
		}
	}
	return failed;
}
