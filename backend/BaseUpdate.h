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
#include <QList>
#include <QUrl>

#include "net/DownloadJob.h"

#include "tasks/Task.h"
#include "libmmc_config.h"

class MinecraftVersion;
class BaseInstance;

/*!
 * The game update task is the task that handles downloading instances' files.
 */
class LIBMULTIMC_EXPORT BaseUpdate : public Task
{
	Q_OBJECT
public:
	explicit BaseUpdate(BaseInstance *inst, QObject *parent = 0);
	
	virtual void executeTask() = 0;

protected slots:
	//virtual void error(const QString &msg);
	void updateDownloadProgress(qint64 current, qint64 total);
	
protected:
	JobListQueue download_queue;
	BaseInstance *m_inst;
};


