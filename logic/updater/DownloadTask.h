/* Copyright 2013-2014 MultiMC Contributors
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

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "GoUpdate.h"

namespace GoUpdate
{
/*!
 * The DownloadTask is a task that takes a given version ID and repository URL,
 * downloads that version's files from the repository, and prepares to install them.
 */
class DownloadTask : public Task
{
	Q_OBJECT

public:
	explicit DownloadTask(Status status, QObject* parent = 0);

	/// Get the directory that will contain the update files.
	QString updateFilesDir();

	/// Get the list of operations that should be done
	OperationList operations();

	/// set updater download behavior
	void setUseLocalUpdater(bool useLocal);

protected:
	//! Entry point for tasks.
	virtual void executeTask() override;

	/*!
	 * Downloads the version info files from the repository.
	 * The files for both the current build, and the build that we're updating to need to be downloaded.
	 * If the current version's info file can't be found, MultiMC will not delete files that
	 * were removed between versions. It will still replace files that have changed, however.
	 * Note that although the repository URL for the current version is not given to the update task,
	 * the task will attempt to look it up in the UpdateChecker's channel list.
	 * If an error occurs here, the function will call emitFailed and return false.
	 */
	void loadVersionInfo();

	NetJobPtr m_vinfoNetJob;
	ByteArrayDownloadPtr m_currentVersionFileListDownload;
	ByteArrayDownloadPtr m_newVersionFileListDownload;

	NetJobPtr m_filesNetJob;

	Status m_status;

	OperationList m_operations;

	/*!
	 * Temporary directory to store update files in.
	 * This will be set to not auto delete. Task will fail if this fails to be created.
	 */
	QTemporaryDir m_updateFilesDir;

protected slots:
	/*!
	 * This function is called when version information is finished downloading
	 * and at least the new file list download succeeded
	 */
	void processDownloadedVersionInfo();
	void vinfoDownloadFailed();

	void fileDownloadFinished();
	void fileDownloadFailed(QString reason);
	void fileDownloadProgressChanged(qint64 current, qint64 total);
};

}