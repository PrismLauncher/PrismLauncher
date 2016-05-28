/* Copyright 2013-2015 MultiMC Contributors
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
#include "Download.h"

#include <QDebug>

void NetJob::partSucceeded(int index)
{
	// do progress. all slots are 1 in size at least
	auto &slot = parts_progress[index];
	partProgress(index, slot.total_progress, slot.total_progress);

	m_doing.remove(index);
	m_done.insert(index);
	downloads[index].get()->disconnect(this);
	startMoreParts();
}

void NetJob::partFailed(int index)
{
	m_doing.remove(index);
	auto &slot = parts_progress[index];
	if (slot.failures == 3)
	{
		m_failed.insert(index);
	}
	else
	{
		slot.failures++;
		m_todo.enqueue(index);
	}
	downloads[index].get()->disconnect(this);
	startMoreParts();
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
	setProgress(current_progress, total_progress);
}

void NetJob::executeTask()
{
	qDebug() << m_job_name.toLocal8Bit() << " started.";
	m_running = true;
	for (int i = 0; i < downloads.size(); i++)
	{
		m_todo.enqueue(i);
	}
	// hack that delays early failures so they can be caught easier
	QMetaObject::invokeMethod(this, "startMoreParts", Qt::QueuedConnection);
}

void NetJob::startMoreParts()
{
	// check for final conditions if there's nothing in the queue
	if(!m_todo.size())
	{
		if(!m_doing.size())
		{
			if(!m_failed.size())
			{
				qDebug() << m_job_name << "succeeded.";
				emitSucceeded();
			}
			else
			{
				qCritical() << m_job_name << "failed.";
				emitFailed(tr("Job '%1' failed to process:\n%2").arg(m_job_name).arg(getFailedFiles().join("\n")));
			}
		}
		return;
	}
	// otherwise try to start more parts
	while (m_doing.size() < 6)
	{
		if(!m_todo.size())
			return;
		int doThis = m_todo.dequeue();
		m_doing.insert(doThis);
		auto part = downloads[doThis];
		// connect signals :D
		connect(part.get(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
		connect(part.get(), SIGNAL(failed(int)), SLOT(partFailed(int)));
		connect(part.get(), SIGNAL(netActionProgress(int, qint64, qint64)),
				SLOT(partProgress(int, qint64, qint64)));
		part->start();
	}
}


QStringList NetJob::getFailedFiles()
{
	QStringList failed;
	for (auto index: m_failed)
	{
		failed.push_back(downloads[index]->m_url.toString());
	}
	failed.sort();
	return failed;
}
