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

#include "Env.h"
#include <minecraft/forge/ForgeXzDownload.h>
#include "OneSixUpdate.h"
#include "OneSixInstance.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>

#include "BaseInstance.h"
#include "minecraft/MinecraftProfile.h"
#include "minecraft/Library.h"
#include "net/URLConstants.h"
#include <FileSystem.h>

#include "update/FoldersTask.h"
#include "update/LibrariesTask.h"
#include "update/FMLLibrariesTask.h"
#include "update/AssetUpdateTask.h"

#include <meta/Index.h>
#include <meta/Version.h>

OneSixUpdate::OneSixUpdate(OneSixInstance *inst, QObject *parent) : Task(parent), m_inst(inst)
{
	// create folders
	{
		m_tasks.append(std::make_shared<FoldersTask>(m_inst));
	}

	// add metadata update tasks, if necessary
	{
		/*
		 * FIXME: there are some corner cases here that remain unhandled:
		 * what if local load succeeds but remote fails? The version is still usable...
		 * We should not rely on the remote to be there... and prefer local files if it does not respond.
		 */
		qDebug() << "Updating patches...";
		auto profile = m_inst->getMinecraftProfile();
		m_inst->reloadProfile();
		for(int i = 0; i < profile->rowCount(); i++)
		{
			auto patch = profile->versionPatch(i);
			auto id = patch->getID();
			auto metadata = patch->getMeta();
			if(metadata)
			{
				metadata->load();
				auto task = metadata->getCurrentTask();
				if(task)
				{
					qDebug() << "Loading remote meta patch" << id;
					m_tasks.append(task.unwrap());
				}
			}
			else
			{
				qDebug() << "Ignoring local patch" << id;
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
	// if the task is already finished by the time we look at it, skip it
	if(task->isFinished())
	{
		next();
	}
	connect(task.get(), &Task::succeeded, this, &OneSixUpdate::subtaskSucceeded);
	connect(task.get(), &Task::failed, this, &OneSixUpdate::subtaskFailed);
	connect(task.get(), &Task::progress, this, &OneSixUpdate::progress);
	connect(task.get(), &Task::status, this, &OneSixUpdate::setStatus);
	// if the task is already running, do not start it again
	if(!task->isRunning())
	{
		task->start();
	}
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
