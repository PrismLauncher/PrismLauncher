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

#pragma once
#include <QtNetwork>
#include <QLabel>
#include "NetAction.h"
#include "ByteArrayDownload.h"
#include "MD5EtagDownload.h"
#include "CacheDownload.h"
#include "HttpMetaCache.h"
#include "logic/tasks/ProgressProvider.h"

class NetJob;
typedef std::shared_ptr<NetJob> NetJobPtr;

class NetJob : public ProgressProvider
{
	Q_OBJECT
public:
	explicit NetJob(QString job_name) : ProgressProvider(), m_job_name(job_name) {}
	virtual ~NetJob() {}
	template <typename T> bool addNetAction(T action)
	{
		NetActionPtr base = std::static_pointer_cast<NetAction>(action);
		base->m_index_within_job = downloads.size();
		downloads.append(action);
		part_info pi;
		{
			pi.current_progress = base->currentProgress();
			pi.total_progress = base->totalProgress();
			pi.failures = base->numberOfFailures();
		}
		parts_progress.append(pi);
		total_progress += pi.total_progress;
		// if this is already running, the action needs to be started right away!
		if (isRunning())
		{
			emit progress(current_progress, total_progress);
			connect(base.get(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
			connect(base.get(), SIGNAL(failed(int)), SLOT(partFailed(int)));
			connect(base.get(), SIGNAL(progress(int, qint64, qint64)),
					SLOT(partProgress(int, qint64, qint64)));
			base->start();
		}
		return true;
	}

	NetActionPtr operator[](int index)
	{
		return downloads[index];
	}
	const NetActionPtr at(const int index)
	{
		return downloads.at(index);
	}
	NetActionPtr first()
	{
		if (downloads.size())
			return downloads[0];
		return NetActionPtr();
	}
	int size() const
	{
		return downloads.size();
	}
	virtual bool isRunning() const
	{
		return m_running;
	}
	QStringList getFailedFiles();
signals:
	void started();
	void progress(qint64 current, qint64 total);
	void succeeded();
	void failed();
public
slots:
	virtual void start();
	// FIXME: implement
	virtual void abort() {};
private
slots:
	void partProgress(int index, qint64 bytesReceived, qint64 bytesTotal);
	void partSucceeded(int index);
	void partFailed(int index);

private:
	struct part_info
	{
		qint64 current_progress = 0;
		qint64 total_progress = 1;
		int failures = 0;
	};
	QString m_job_name;
	QList<NetActionPtr> downloads;
	QList<part_info> parts_progress;
	qint64 current_progress = 0;
	qint64 total_progress = 0;
	int num_succeeded = 0;
	int num_failed = 0;
	bool m_running = false;
};
