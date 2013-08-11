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
#include "BaseUpdate.h"

class MinecraftVersion;
class BaseInstance;

class LIBMULTIMC_EXPORT OneSixUpdate : public BaseUpdate
{
	Q_OBJECT
public:
	explicit OneSixUpdate(BaseInstance *inst, QObject *parent = 0);
	virtual void executeTask();
	
private slots:
	void versionFileStart();
	void versionFileFinished();
	void versionFileFailed();
	
	void jarlibStart();
	void jarlibFinished();
	void jarlibFailed();
	
private:
	JobListPtr legacyDownloadJob;
	JobListPtr specificVersionDownloadJob;
	JobListPtr jarlibDownloadJob;
	JobListQueue download_queue;
	
	// target version, determined during this task
	QSharedPointer<MinecraftVersion> targetVersion;
};


