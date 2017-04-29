/* Copyright 2013-2017 MultiMC Contributors
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

void NetJob::partSucceeded()
{
	auto index = m_partsIndex[(NetAction *)QObject::sender()];
	// do progress. all slots are 1 in size at least
	auto &slot = m_parts[index];
	setPartProgress(index, slot.total_progress, slot.total_progress);

	m_doing.remove(index);
	m_done.insert(index);
	slot.download->disconnect(this);
	startMoreParts();
}

void NetJob::partFailed()
{
	auto index = m_partsIndex[(NetAction *)QObject::sender()];
	m_doing.remove(index);
	auto &slot = m_parts[index];
	if (slot.failures == 3)
	{
		m_failed.insert(index);
	}
	else
	{
		slot.failures++;
		m_todo.enqueue(index);
	}
	slot.download->disconnect(this);
	startMoreParts();
}

void NetJob::partAborted()
{
	auto index = m_partsIndex[(NetAction *)QObject::sender()];
	m_aborted = true;
	m_doing.remove(index);
	auto &slot = m_parts[index];
	m_failed.insert(index);
	slot.download->disconnect(this);
	startMoreParts();
}

void NetJob::partProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	auto index = m_partsIndex[(NetAction *)QObject::sender()];
	setPartProgress(index, bytesReceived, bytesTotal);
}

void NetJob::setPartProgress(int index, qint64 bytesReceived, qint64 bytesTotal)
{
	auto &slot = m_parts[index];

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
	for (int i = 0; i < m_parts.size(); i++)
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
			else if(m_aborted)
			{
				qDebug() << m_job_name << "aborted.";
				emitFailed(tr("Job '%1' aborted.").arg(m_job_name));
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
		auto part = m_parts[doThis].download;
		// connect signals :D
		connectAction(part.get());
		part->start();
	}
}

void NetJob::connectAction(NetAction* action)
{
	connect(action, &NetAction::succeeded, this, &NetJob::partSucceeded);
	connect(action, &NetAction::failed, this, &NetJob::partFailed);
	connect(action, &NetAction::aborted, this, &NetJob::partAborted);
	connect(action, &NetAction::progress, this, &NetJob::partProgress);
}


QStringList NetJob::getFailedFiles()
{
	QStringList failed;
	for (auto index: m_failed)
	{
		failed.push_back(m_parts[index].download->m_url.toString());
	}
	failed.sort();
	return failed;
}

bool NetJob::canAbort() const
{
	bool canFullyAbort = true;
	// can abort the waiting?
	for(auto index: m_todo)
	{
		auto part = m_parts[index].download;
		canFullyAbort &= part->canAbort();
	}
	// can abort the active?
	for(auto index: m_doing)
	{
		auto part = m_parts[index].download;
		canFullyAbort &= part->canAbort();
	}
	return canFullyAbort;
}

bool NetJob::abort()
{
	bool fullyAborted = true;
	// fail all waiting
	m_failed.unite(m_todo.toSet());
	m_todo.clear();
	// abort active
	auto toKill = m_doing.toList();
	for(auto index: toKill)
	{
		auto part = m_parts[index].download;
		fullyAborted &= part->abort();
	}
	return fullyAborted;
}

void NetJob::addNetAction(NetActionPtr action)
{
	m_partsIndex[action.get()] = m_parts.count();
	part_info pi;
	{
		pi.current_progress = action->getProgress();
		pi.total_progress = action->getTotalProgress();
		pi.failures = 0;
		pi.download = action;
	}
	m_parts.append(pi);

	total_progress += pi.total_progress;
	current_progress += pi.current_progress;

	// if this is already running, the action needs to be started right away!
	if (isRunning())
	{
		setProgress(current_progress, total_progress);
		connectAction(action.get());
		action->start();
	}
}
