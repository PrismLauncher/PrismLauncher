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

#include "task.h"

Task::Task(QObject *parent) :
	QThread(parent)
{
	
}

QString Task::getStatus() const
{
	return status;
}

void Task::setStatus(const QString &status)
{
	this->status = status;
	statusChanged(status);
}

int Task::getProgress() const
{
	return progress;
}

void Task::setProgress(int progress)
{
	this->progress = progress;
	progressChanged(progress);
}

void Task::startTask()
{
	start();
}

void Task::run()
{
	taskStarted(this);
	executeTask();
	taskEnded(this);
}
