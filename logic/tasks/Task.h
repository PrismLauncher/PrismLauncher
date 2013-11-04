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

#include <QObject>
#include <QString>
#include "ProgressProvider.h"

class Task : public ProgressProvider
{
	Q_OBJECT
public:
	explicit Task(QObject *parent = 0);

	virtual QString getStatus() const;
	virtual void getProgress(qint64 &current, qint64 &total);
	virtual bool isRunning() const;

public
slots:
	virtual void start();

protected:
	virtual void executeTask() = 0;

	virtual void emitSucceeded();
	virtual void emitFailed(QString reason);

protected
slots:
	void setStatus(const QString &status);
	void setProgress(int progress);

protected:
	QString m_status;
	int m_progress = 0;
	bool m_running = false;
};
