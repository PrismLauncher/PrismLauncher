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

#include <QNetworkAccessManager>
#include <QUrl>
#include "dlqueue.h"

#include "tasks/Task.h"
#include "libmmc_config.h"

class MinecraftVersion;
class BaseInstance;

/*!
 * The game update task is the task that handles downloading instances' files.
 */
class LIBMULTIMC_EXPORT OneSixUpdate : public Task
{
	Q_OBJECT
public:
	explicit OneSixUpdate(BaseInstance *inst, QObject *parent = 0);
	
	virtual void executeTask();
	
public slots:
	virtual void error(const QString &msg);
	
private slots:
	void updateDownloadProgress(qint64 current, qint64 total);
	
	void versionFileStart();
	void versionFileFinished();
	void versionFileFailed();
	
	void jarlibStart();
	void jarlibFinished();
	void jarlibFailed();
	
	
signals:
	/*!
	 * \brief Signal emitted when the game update is complete.
	 */
	void gameUpdateComplete();
	
	/*!
	 * \brief Signal emitted if an error occurrs during the update.
	 * \param errorMsg An error message to be displayed to the user.
	 */
	void gameUpdateError(const QString &errorMsg);
	
private:
	BaseInstance *m_inst;

	QString m_subStatusMsg;
	
	QSharedPointer<QNetworkAccessManager> net_manager {new QNetworkAccessManager()};
	JobListPtr legacyDownloadJob;
	JobListPtr specificVersionDownloadJob;
	JobListPtr jarlibDownloadJob;
	JobListQueue download_queue;
	
	// target version, determined during this task
	MinecraftVersion *targetVersion;
};


