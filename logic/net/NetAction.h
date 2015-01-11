/* Copyright 2013-2014 MultiMC Contributors
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

#include <QObject>
#include <QUrl>
#include <memory>
#include <QNetworkReply>

enum JobStatus
{
	Job_NotStarted,
	Job_InProgress,
	Job_Finished,
	Job_Failed
};

typedef std::shared_ptr<class NetAction> NetActionPtr;
class NetAction : public QObject, public std::enable_shared_from_this<NetAction>
{
	Q_OBJECT
protected:
	explicit NetAction() : QObject(0) {};

public:
	virtual ~NetAction() {};

public:
	virtual qint64 totalProgress() const
	{
		return m_total_progress;
	}
	virtual qint64 currentProgress() const
	{
		return m_progress;
	}
	virtual qint64 numberOfFailures() const
	{
		return m_failures;
	}
	NetActionPtr getSharedPtr()
	{
		return shared_from_this();
	}

public:
	/// the network reply
	std::shared_ptr<QNetworkReply> m_reply;

	/// the content of the content-type header
	QString m_content_type;

	/// source URL
	QUrl m_url;

	/// The file's status
	JobStatus m_status = Job_NotStarted;

	/// index within the parent job
	int m_index_within_job = 0;

	qint64 m_progress = 0;
	qint64 m_total_progress = 1;

	/// number of failures up to this point
	int m_failures = 0;

signals:
	void started(int index);
	void progress(int index, qint64 current, qint64 total);
	void succeeded(int index);
	void failed(int index);

protected
slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) = 0;
	virtual void downloadError(QNetworkReply::NetworkError error) = 0;
	virtual void downloadFinished() = 0;
	virtual void downloadReadyRead() = 0;

public
slots:
	virtual void start() = 0;
};
