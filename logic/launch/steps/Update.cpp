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

#include "Update.h"
#include <launch/LaunchTask.h>

void Update::executeTask()
{
	m_updateTask = m_parent->instance()->createUpdateTask();
	if(m_updateTask)
	{
		connect(m_updateTask.get(), SIGNAL(finished()), this, SLOT(updateFinished()));
		connect(m_updateTask.get(), &Task::progress, this, &Task::setProgress);
		connect(m_updateTask.get(), &Task::status, this, &Task::setStatus);
		emit progressReportingRequest();
		return;
	}
	emitSucceeded();
}

void Update::proceed()
{
	m_updateTask->start();
}

void Update::updateFinished()
{
	if(m_updateTask->successful())
	{
		emitSucceeded();
	}
	else
	{
		QString reason = tr("Instance update failed because: %1.\n\n").arg(m_updateTask->failReason());
		emit logLine(reason, MessageLevel::Fatal);
		emitFailed(reason);
	}
}
