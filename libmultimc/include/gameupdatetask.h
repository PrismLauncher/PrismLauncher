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

#ifndef GAMEUPDATETASK_H
#define GAMEUPDATETASK_H

#include <QObject>

#include <QList>

#include <QNetworkAccessManager>
#include <QUrl>

#include "task.h"
#include "loginresponse.h"
#include "instance.h"

#include "libmmc_config.h"
class FileToDownload;
typedef QSharedPointer<FileToDownload> PtrFileToDownload;

class FileToDownload : public QObject
{
	Q_OBJECT
	
	/*!
	 * The URL to download the file from.
	 */
	Q_PROPERTY(QUrl url READ url WRITE setURL)
	
	/*!
	 * The path to download to.
	 * This path is relative to the instance's root directory.
	 */
	Q_PROPERTY(QString path READ path WRITE setPath)
	
private:
	FileToDownload(const QUrl &url, const QString &path, QObject *parent = 0);
public:
	static PtrFileToDownload Create(const QUrl &url, const QString &path, QObject *parent = 0);
	
	virtual QUrl url() const { return m_dlURL; }
	virtual void setURL(const QUrl &url) { m_dlURL = url; }
	
	virtual QString path() const { return m_dlPath; }
	virtual void setPath(const QString &path) { m_dlPath = path; }
	
private:
	QUrl m_dlURL;
	QString m_dlPath;
};



/*!
 * The game update task is the task that handles downloading instances' files.
 */
class LIBMULTIMC_EXPORT GameUpdateTask : public Task
{
	Q_OBJECT
	
	/*!
	 * The task's state.
	 * A certain state message will be shown depending on what this is set to.
	 */
	Q_PROPERTY(int state READ state WRITE setState)
	
	/*!
	 * The substatus message.
	 * This will be next to the the state message in the task's status.
	 */
	Q_PROPERTY(QString subStatus READ subStatus WRITE setSubStatus)
public:
	explicit GameUpdateTask(const LoginResponse &response, Instance *inst, QObject *parent = 0);
	
	
	/////////////////////////
	// EXECUTION FUNCTIONS //
	/////////////////////////
	
	virtual void executeTask();
	
	virtual bool downloadFile(const PtrFileToDownload file);
	
	
	//////////////////////
	// STATE AND STATUS //
	//////////////////////
	
	virtual int state() const;
	virtual void setState(int state, bool resetSubStatus = true);
	
	virtual QString subStatus() const;
	virtual void setSubStatus(const QString &msg);
	
	/*!
	 * Gets the message that will be displated for the given state.
	 */
	virtual QString getStateMessage(int state);
	
public slots:
	
	/*!
	 * Updates the status message based on the state and substatus message.
	 */
	virtual void updateStatus();
	
	
	virtual void error(const QString &msg);
	
	
private slots:
	virtual void updateDownloadProgress(qint64 current, qint64 total);
	
signals:
	/*!
	 * \brief Signal emitted when the game update is complete.
	 * \param response The login response received from login task.
	 */
	void gameUpdateComplete(const LoginResponse &response);
	
	/*!
	 * \brief Signal emitted if an error occurrs during the update.
	 * \param errorMsg An error message to be displayed to the user.
	 */
	void gameUpdateError(const QString &errorMsg);
	
private:
	///////////
	// STUFF //
	///////////
	
	Instance *m_inst;
	
	LoginResponse m_response;
	
	QNetworkAccessManager *netMgr;
	
	
	
	////////////////////////
	// FILE DOWNLOAD LIST //
	////////////////////////
	
	// List of URLs that the game updater will need to download.
	QList<PtrFileToDownload> m_downloadList;
	int m_currentDownload;
	
	
	
	////////////////////////////
	// STATE AND STATUS STUFF //
	////////////////////////////
	
	int m_updateState;
	QString m_subStatusMsg;
	
	enum UpdateState
	{
		// Initializing
		StateInit = 0,
		
		// Determining files to download
		StateDetermineURLs,
		
		// Downloading files
		StateDownloadFiles,
		
		// Installing files
		StateInstall,
		
		// Finished
		StateFinished
	};
};


#endif // GAMEUPDATETASK_H
