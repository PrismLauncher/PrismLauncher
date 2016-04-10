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

#include "ModMinecraftJar.h"
#include <launch/LaunchTask.h>
#include <QStandardPaths>

void ModMinecraftJar::executeTask()
{
	m_jarModTask = m_parent->instance()->createJarModdingTask();
	if(m_jarModTask)
	{
		connect(m_jarModTask.get(), SIGNAL(finished()), this, SLOT(jarModdingFinished()));
		m_jarModTask->start();
		return;
	}
	emitSucceeded();
}

void ModMinecraftJar::jarModdingFinished()
{
	if(m_jarModTask->successful())
	{
		emitSucceeded();
	}
	else
	{
		QString reason = tr("jar modding failed because: %1.\n\n").arg(m_jarModTask->failReason());
		emit logLine(reason, MessageLevel::Fatal);
		emitFailed(reason);
	}
}
