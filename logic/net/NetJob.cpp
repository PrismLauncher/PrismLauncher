/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "NetJob.h"
#include "pathutils.h"
#include "MultiMC.h"
#include "MD5EtagDownload.h"
#include "ByteArrayDownload.h"
#include "CacheDownload.h"

#include "logger/QsLog.h"

void NetJob::partSucceeded(int index)
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

void NetJob::partFailed(int index)
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

void NetJob::partProgress(int index, qint64 bytesReceived, qint64 bytesTotal)
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

void NetJob::start()
{
	QLOG_INFO() << m_job_name.toLocal8Bit() << " started.";
	m_running = true;
	for (auto iter : downloads)
	{
		connect(iter.get(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
		connect(iter.get(), SIGNAL(failed(int)), SLOT(partFailed(int)));
		connect(iter.get(), SIGNAL(progress(int, qint64, qint64)),
				SLOT(partProgress(int, qint64, qint64)));
		iter->start();
	}
}

QStringList NetJob::getFailedFiles()
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
