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
#include "MinecraftUpdate.h"
#include "MinecraftInstance.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>

#include "BaseInstance.h"
#include "minecraft/ComponentList.h"
#include "minecraft/Library.h"
#include "net/URLConstants.h"
#include <FileSystem.h>

#include "update/FoldersTask.h"
#include "update/LibrariesTask.h"
#include "update/FMLLibrariesTask.h"
#include "update/AssetUpdateTask.h"

#include <meta/Index.h>
#include <meta/Version.h>

OneSixUpdate::OneSixUpdate(MinecraftInstance *inst, QObject *parent) : Task(parent), m_inst(inst)
{
}

void OneSixUpdate::executeTask()
{
	m_tasks.clear();
	// create folders
	{
		m_tasks.append(std::make_shared<FoldersTask>(m_inst));
	}

	// add metadata update task if necessary
	{
		auto components = m_inst->getComponentList();
		components->reload(Net::Mode::Online);
		auto task = components->getCurrentTask();
		if(task)
		{
			m_tasks.append(task.unwrap());
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
	if(m_failed_out_of_order)
	{
		emitFailed(m_fail_reason);
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
		qCritical() << "OneSixUpdate: Skipping finished subtask" << m_currentTask << ":" << task.get();
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
	if(isFinished())
	{
		qCritical() << "OneSixUpdate: Subtask" << sender() << "succeeded, but work was already done!";
		return;
	}
	auto senderTask = QObject::sender();
	auto currentTask = m_tasks[m_currentTask].get();
	if(senderTask != currentTask)
	{
		qDebug() << "OneSixUpdate: Subtask" << sender() << "succeeded out of order.";
		return;
	}
	next();
}

void OneSixUpdate::subtaskFailed(QString error)
{
	if(isFinished())
	{
		qCritical() << "OneSixUpdate: Subtask" << sender() << "failed, but work was already done!";
		return;
	}
	auto senderTask = QObject::sender();
	auto currentTask = m_tasks[m_currentTask].get();
	if(senderTask != currentTask)
	{
		qDebug() << "OneSixUpdate: Subtask" << sender() << "failed out of order.";
		m_failed_out_of_order = true;
		m_fail_reason = error;
		return;
	}
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
