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
	QObject(parent)
{
	
}

QString Task::getStatus() const
{
	return status;
}

void Task::setStatus(const QString &status)
{
	this->status = status;
	emitStatusChange(status);
}

int Task::getProgress() const
{
	return progress;
}

void Task::setProgress(int progress)
{
	this->progress = progress;
	emitProgressChange(progress);
}

void Task::startTask()
{
	emitStarted();
	executeTask();
}

void Task::emitStarted()
{
	running = true;
	emit started();
}

void Task::emitFailed(QString reason)
{
	running = false;
	emit failed(reason);
}

void Task::emitSucceeded()
{
	running = false;
	emit succeeded();
}


bool Task::isRunning() const
{
	return running;
}


void Task::emitStatusChange(const QString &status)
{
	emit statusChanged(status);
}

void Task::emitProgressChange(int progress)
{
	emit progressChanged(progress);
}
