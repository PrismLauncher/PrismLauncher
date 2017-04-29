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

#pragma once

#include <QObject>
#include <QString>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT Task : public QObject
{
	Q_OBJECT
public:
	enum class Status
	{
		NotStarted,
		InProgress,
		Finished,
		Failed,
		Aborted,
		Failed_Proceed
	};
public:
	explicit Task(QObject *parent = 0);
	virtual ~Task() {};

	virtual bool isRunning() const;

	virtual bool isFinished() const;

	/*!
	 * True if this task was successful.
	 * If the task failed or is still running, returns false.
	 */
	virtual bool successful() const;

	/*!
	 * Returns the string that was passed to emitFailed as the error message when the task failed.
	 * If the task hasn't failed, returns an empty string.
	 */
	virtual QString failReason() const;

	virtual bool canAbort() const
	{
		return false;
	}

	QString getStatusText()
	{
		return m_statusText;
	}

	virtual qint64 getProgress()
	{
		return m_progress;
	}

	virtual qint64 getTotalProgress()
	{
		return m_progressTotal;
	}

signals:
	void started();
	void progress(qint64 current, qint64 total);
	void finished();
	void succeeded();
	void failed(QString reason);
	void status(QString status);

public slots:
	virtual void start();
	virtual bool abort()
	{
		return false;
	};

protected:
	virtual void executeTask() = 0;

protected slots:
	virtual void emitSucceeded();
	virtual void emitFailed(QString reason);

public slots:
	void setStatusText(const QString &status);
	void setProgress(qint64 current, qint64 total);

protected:
	// FIXME: replace these with the m_status from NetAction
	bool m_running = false;
	bool m_finished = false;
	bool m_succeeded = false;
	QString m_failReason = "";
	QString m_statusText;

	qint64 m_progress = 0;
	qint64 m_progressTotal = 1;
};

