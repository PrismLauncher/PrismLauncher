#include "DownloadJob.h"
#include "pathutils.h"
#include "MultiMC.h"
#include "FileDownload.h"
#include "ByteArrayDownload.h"
#include "CacheDownload.h"

#include <logger/QsLog.h>

ByteArrayDownloadPtr DownloadJob::addByteArrayDownload(QUrl url)
{
	ByteArrayDownloadPtr ptr(new ByteArrayDownload(url));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	parts_progress.append(part_info());
	total_progress++;
	return ptr;
}

FileDownloadPtr DownloadJob::addFileDownload(QUrl url, QString rel_target_path)
{
	FileDownloadPtr ptr(new FileDownload(url, rel_target_path));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	parts_progress.append(part_info());
	total_progress++;
	return ptr;
}

CacheDownloadPtr DownloadJob::addCacheDownload(QUrl url, MetaEntryPtr entry)
{
	CacheDownloadPtr ptr(new CacheDownload(url, entry));
	ptr->index_within_job = downloads.size();
	downloads.append(ptr);
	parts_progress.append(part_info());
	total_progress++;
	return ptr;
}

ForgeXzDownloadPtr DownloadJob::addForgeXzDownload(QUrl url, MetaEntryPtr entry)
{
	ForgeXzDownloadPtr ptr(new ForgeXzDownload(url, entry));
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
	QLOG_INFO() << m_job_name.toLocal8Bit() << "progress:" << num_succeeded << "/"
			 << downloads.size();
	if (num_failed + num_succeeded == downloads.size())
	{
		if (num_failed)
		{
			QLOG_ERROR() << m_job_name.toLocal8Bit() << "failed.";
			emit failed();
		}
		else
		{
			QLOG_INFO() << m_job_name.toLocal8Bit() << "succeeded.";
			emit succeeded();
		}
	}
}

void DownloadJob::partFailed(int index)
{
	auto &slot = parts_progress[index];
	if (slot.failures == 3)
	{
		QLOG_ERROR() << "Part" << index << "failed 3 times (" << downloads[index]->m_url << ")";
		num_failed++;
		if (num_failed + num_succeeded == downloads.size())
		{
			QLOG_ERROR() << m_job_name.toLocal8Bit() << "failed.";
			emit failed();
		}
	}
	else
	{
		QLOG_ERROR() << "Part" << index << "failed, restarting (" << downloads[index]->m_url
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
	QLOG_INFO() << m_job_name.toLocal8Bit() << " started.";
	for (auto iter : downloads)
	{
		connect(iter.get(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
		connect(iter.get(), SIGNAL(failed(int)), SLOT(partFailed(int)));
		connect(iter.get(), SIGNAL(progress(int, qint64, qint64)),
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
