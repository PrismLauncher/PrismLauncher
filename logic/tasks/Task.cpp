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

#include "Task.h"

Task::Task(QObject *parent) :
	ProgressProvider(parent)
{
	
}

QString Task::getStatus() const
{
	return m_status;
}

void Task::setStatus(const QString &new_status)
{
	m_status = new_status;
	emit status(new_status);
}

void Task::setProgress(int new_progress)
{
	m_progress = new_progress;
	emit progress(new_progress, 100);
}

void Task::getProgress(qint64& current, qint64& total)
{
	current = m_progress;
	total = 100;
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
	emit failed(reason);
}

void Task::emitSucceeded()
{
	m_running = false;
	emit succeeded();
}


bool Task::isRunning() const
{
	return m_running;
}
