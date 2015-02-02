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

#include "Task.h"
#include <QDebug>

Task::Task(QObject *parent) : ProgressProvider(parent)
{
}

void Task::setStatus(const QString &new_status)
{
	emit status(new_status);
}

void Task::setProgress(int new_progress)
{
	emit progress(new_progress, 100);
}

void Task::start()
{
	m_running = true;
	emit started();
	executeTask();
}

void Task::emitFailed(QString reason)
{
	m_running = false;
	m_succeeded = false;
	m_failReason = reason;
	qCritical() << "Task failed: " << reason;
	emit failed(reason);
}

void Task::emitSucceeded()
{
	if (!m_running) { return; } // Don't succeed twice.
	m_running = false;
	m_succeeded = true;
	qDebug() << "Task succeeded";
	emit succeeded();
}

bool Task::isRunning() const
{
	return m_running;
}

bool Task::successful() const
{
	return m_succeeded;
}

QString Task::failReason() const
{
	return m_failReason;
}

