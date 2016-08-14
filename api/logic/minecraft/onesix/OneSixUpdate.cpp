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

#include "Env.h"
#include <minecraft/forge/ForgeXzDownload.h>
#include "OneSixUpdate.h"
#include "OneSixInstance.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>

#include "BaseInstance.h"
#include "minecraft/MinecraftVersionList.h"
#include "minecraft/MinecraftProfile.h"
#include "minecraft/Library.h"
#include "net/URLConstants.h"
#include <FileSystem.h>

#include "update/FoldersTask.h"
#include "update/LibrariesTask.h"
#include "update/FMLLibrariesTask.h"
#include "update/AssetUpdateTask.h"

OneSixUpdate::OneSixUpdate(OneSixInstance *inst, QObject *parent) : Task(parent), m_inst(inst)
{
	// create folders
	{
		m_tasks.append(std::make_shared<FoldersTask>(m_inst));
	}

	// add a version update task, if necessary
	{
		auto list = std::dynamic_pointer_cast<MinecraftVersionList>(ENV.getVersionList("net.minecraft"));
		auto version = std::dynamic_pointer_cast<MinecraftVersion>(list->findVersion(m_inst->intendedVersionId()));
		if (version == nullptr)
		{
			// don't do anything if it was invalid
			m_preFailure = tr("The specified Minecraft version is invalid. Choose a different one.");
		}
		else if (m_inst->providesVersionFile() || !version->needsUpdate())
		{
			qDebug() << "Instance either provides a version file or doesn't need an update.";
		}
		else
		{
			auto versionUpdateTask = list->createUpdateTask(m_inst->intendedVersionId());
			if (!versionUpdateTask)
			{
				qDebug() << "Didn't spawn an update task.";
			}
			else
			{
				m_tasks.append(versionUpdateTask);
			}
		}
	}

	// libraries download
	{
		m_tasks.append(std::make_shared<LibrariesTask>(m_inst));
	}

	// FML libraries download and copy into the instance
	{
		m_tasks.append(std::make_shared<FMLLibrariesTask>(m_inst));
	}

	// assets update
	{
		m_tasks.append(std::make_shared<AssetUpdateTask>(m_inst));
	}
}

void OneSixUpdate::executeTask()
{
	if(!m_preFailure.isEmpty())
	{
		emitFailed(m_preFailure);
		return;
	}
	next();
}

void OneSixUpdate::next()
{
	if(m_abort)
	{
		emitFailed(tr("Aborted by user."));
		return;
	}
	m_currentTask ++;
	if(m_currentTask > 0)
	{
		auto task = m_tasks[m_currentTask - 1];
		disconnect(task.get(), &Task::succeeded, this, &OneSixUpdate::subtaskSucceeded);
		disconnect(task.get(), &Task::failed, this, &OneSixUpdate::subtaskFailed);
		disconnect(task.get(), &Task::progress, this, &OneSixUpdate::progress);
		disconnect(task.get(), &Task::status, this, &OneSixUpdate::setStatus);
	}
	if(m_currentTask == m_tasks.size())
	{
		emitSucceeded();
		return;
	}
	auto task = m_tasks[m_currentTask];
	connect(task.get(), &Task::succeeded, this, &OneSixUpdate::subtaskSucceeded);
	connect(task.get(), &Task::failed, this, &OneSixUpdate::subtaskFailed);
	connect(task.get(), &Task::progress, this, &OneSixUpdate::progress);
	connect(task.get(), &Task::status, this, &OneSixUpdate::setStatus);
	task->start();
}

void OneSixUpdate::subtaskSucceeded()
{
	next();
}

void OneSixUpdate::subtaskFailed(QString error)
{
	emitFailed(error);
}


bool OneSixUpdate::abort()
{
	if(!m_abort)
	{
		m_abort = true;
		auto task = m_tasks[m_currentTask];
		if(task->canAbort())
		{
			return task->abort();
		}
	}
	return true;
}

bool OneSixUpdate::canAbort() const
{
	return true;
}
