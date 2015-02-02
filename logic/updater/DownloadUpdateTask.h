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

#pragma once

#include "logic/tasks/Task.h"
#include "logic/net/NetJob.h"

/*!
 * The DownloadUpdateTask is a task that takes a given version ID and repository URL,
 * downloads that version's files from the repository, and prepares to install them.
 */
class DownloadUpdateTask : public Task
{
	Q_OBJECT

public:
	explicit DownloadUpdateTask(QString repoUrl, int versionId, QObject* parent=0);

	/*!
	 * Gets the directory that contains the update files.
	 */
	QString updateFilesDir();
	
public:

	// TODO: We should probably put these data structures into a separate header...

	/*!
	 * Struct that describes an entry in a VersionFileEntry's `Sources` list.
	 */
	struct FileSource
	{
		FileSource(QString type, QString url, QString compression="")
		{
			this->type = type;
			this->url = url;
			this->compressionType = compression;
		}

		QString type;
		QString url;
		QString compressionType;
	};
	typedef QList<FileSource> FileSourceList;

	/*!
	 * Structure that describes an entry in a GoUpdate version's `Files` list.
	 */
	struct VersionFileEntry
	{
		QString path;
		int mode;
		FileSourceList sources;
		QString md5;
	};
	typedef QList<VersionFileEntry> VersionFileList;

	/*!
	 * Structure that describes an operation to perform when installing updates.
	 */
	struct UpdateOperation
	{
		static UpdateOperation CopyOp(QString fsource, QString fdest, int fmode=0644) { return UpdateOperation{OP_COPY, fsource, fdest, fmode}; }
		static UpdateOperation MoveOp(QString fsource, QString fdest, int fmode=0644) { return UpdateOperation{OP_MOVE, fsource, fdest, fmode}; }
		static UpdateOperation DeleteOp(QString file) { return UpdateOperation{OP_DELETE, file, "", 0644}; }
		static UpdateOperation ChmodOp(QString file, int fmode) { return UpdateOperation{OP_CHMOD, file, "", fmode}; }

		//! Specifies the type of operation that this is.
		enum Type
		{
			OP_COPY,
			OP_DELETE,
			OP_MOVE,
			OP_CHMOD,
		} type;

		//! The file to operate on. If this is a DELETE or CHMOD operation, this is the file that will be modified.
		QString file;

		//! The destination file. If this is a DELETE or CHMOD operation, this field will be ignored.
		QString dest;

		//! The mode to change the source file to. Ignored if this isn't a CHMOD operation.
		int mode;

		// Yeah yeah, polymorphism blah blah inheritance, blah blah object oriented. I'm lazy, OK?
	};
	typedef QList<UpdateOperation> UpdateOperationList;

protected:
	friend class DownloadUpdateTaskTest;


	/*!
	 * Used for arguments to parseVersionInfo and friends to specify which version info file to parse.
	 */
	enum VersionInfoFileEnum { NEW_VERSION, CURRENT_VERSION };


	//! Entry point for tasks.
	virtual void executeTask();

	/*!
	 * Attempts to find the version ID and repository URL for the current version.
	 * The function will look up the repository URL in the UpdateChecker's channel list.
	 * If the repository URL can't be found, this function will return false.
	 */
	virtual void findCurrentVersionInfo();

	/*!
	 * This runs after we've tried loading the channel list.
	 * If the channel list doesn't need to be loaded, this will be called immediately.
	 * If the channel list does need to be loaded, this will be called when it's done.
	 */
	void processChannels();

	/*!
	 * Downloads the version info files from the repository.
	 * The files for both the current build, and the build that we're updating to need to be downloaded.
	 * If the current version's info file can't be found, MultiMC will not delete files that 
	 * were removed between versions. It will still replace files that have changed, however.
	 * Note that although the repository URL for the current version is not given to the update task,
	 * the task will attempt to look it up in the UpdateChecker's channel list.
	 * If an error occurs here, the function will call emitFailed and return false.
	 */
	virtual void loadVersionInfo();

	/*!
	 * This function is called when version information is finished downloading.
	 * This handles parsing the JSON downloaded by the version info network job and then calls processFileLists.
	 * Note that this function will sometimes be called even if the version info download emits failed. If
	 * we couldn't download the current version's info file, we can still update. This will be called even if the 
	 * current version's info file fails to download, as long as the new version's info file succeeded.
	 */
	virtual void parseDownloadedVersionInfo();

	/*!
	 * Loads the file list from the given version info JSON object into the given list.
	 */
	virtual bool parseVersionInfo(const QByteArray &data, VersionFileList* list, QString *error);

	/*!
	 * Takes a list of file entries for the current version's files and the new version's files
	 * and populates the downloadList and operationList with information about how to download and install the update.
	 */
	virtual bool processFileLists(NetJob *job, const VersionFileList &currentVersion, const VersionFileList &newVersion, UpdateOperationList &ops);

	/*!
	 * Calls \see processFileLists to populate the \see m_operationList and a NetJob, and then executes
	 * the NetJob to fetch all needed files
	 */
	virtual void processFileLists();

	/*!
	 * Takes the operations list and writes an install script for the updater to the update files directory.
	 */
	virtual bool writeInstallScript(UpdateOperationList& opsList, QString scriptFile);

	UpdateOperationList m_operationList;

	VersionFileList m_nVersionFileList;
	VersionFileList m_cVersionFileList;

	//! Network job for downloading version info files.
	NetJobPtr m_vinfoNetJob;
	
	//! Network job for downloading update files.
	NetJobPtr m_filesNetJob;

	// Version ID and repo URL for the new version.
	int m_nVersionId;
	QString m_nRepoUrl;

	// Version ID and repo URL for the currently installed version.
	int m_cVersionId;
	QString m_cRepoUrl;

	/*!
	 * Temporary directory to store update files in.
	 * This will be set to not auto delete. Task will fail if this fails to be created.
	 */
	QTemporaryDir m_updateFilesDir;

	/*!
	 * Filters paths
	 * This fixes destination paths for OSX.
	 * The updater runs in MultiMC.app/Contents/MacOs by default
	 * The destination paths are such as this: MultiMC.app/blah/blah
	 * 
	 * Therefore we chop off the 'MultiMC.app' prefix
	 * 
	 * Returns false if the path couldn't be fixed (is invalid)
	 */
	static bool fixPathForOSX(QString &path);

protected slots:
	void vinfoDownloadFinished();
	void vinfoDownloadFailed();

	void fileDownloadFinished();
	void fileDownloadFailed();
	void fileDownloadProgressChanged(qint64 current, qint64 total);
};

