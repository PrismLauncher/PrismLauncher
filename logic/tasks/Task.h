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
#include <QString>
#include "ProgressProvider.h"

class Task : public ProgressProvider
{
	Q_OBJECT
public:
	explicit Task(QObject *parent = 0);
	virtual ~Task() {};

	virtual bool isRunning() const;

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

public
slots:
	virtual void start();
	virtual void abort() {};

protected:
	virtual void executeTask() = 0;

protected slots:
	virtual void emitSucceeded();
	virtual void emitFailed(QString reason);

protected
slots:
	void setStatus(const QString &status);
	void setProgress(int progress);

protected:
	bool m_running = false;
	bool m_succeeded = false;
	QString m_failReason = "";
};

