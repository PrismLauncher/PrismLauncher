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

#include "gameupdatetask.h"

#include <QtNetwork>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>

#include <QDebug>

#include "minecraftversionlist.h"

#include "pathutils.h"
#include "netutils.h"

GameUpdateTask::GameUpdateTask(const LoginResponse &response, Instance *inst, QObject *parent) :
	Task(parent), m_response(response)
{
	m_inst = inst;
	m_updateState = StateInit;
	m_currentDownload = 0;
}

void GameUpdateTask::executeTask()
{
	updateStatus();
	
	QNetworkAccessManager networkMgr;
	netMgr = &networkMgr;
	
	// Get a pointer to the version object that corresponds to the instance's version.
	MinecraftVersion *targetVersion = (MinecraftVersion *)MinecraftVersionList::getMainList().
			findVersion(m_inst->intendedVersion());
	if(targetVersion == NULL)
	{
		//Q_ASSERT_X(targetVersion != NULL, "game update", "instance's intended version is not an actual version");
		setState(StateFinished);
		emit gameUpdateComplete(m_response);
		return;
	}
	
	// Make directories
	QDir binDir(m_inst->binDir());
	if (!binDir.exists() && !binDir.mkpath("."))
	{
		error("Failed to create bin folder.");
		return;
	}
	
	
	
	/////////////////////////
	// BUILD DOWNLOAD LIST //
	/////////////////////////
	// Build a list of URLs that will need to be downloaded.
	
	setState(StateDetermineURLs);
	
	
	// Add the URL for minecraft.jar
	
	// This will be either 'minecraft' or the version number, depending on where
	// we're downloading from.
	QString jarFilename = "minecraft";
	
	// FIXME: this is NOT enough
	if (targetVersion->launcherVersion() == MinecraftVersion::Launcher16)
		jarFilename = targetVersion->descriptor();
	
	QUrl mcJarURL = targetVersion->downloadURL() + jarFilename + ".jar";
	qDebug() << mcJarURL.toString();
	m_downloadList.append(FileToDownload::Create(mcJarURL, PathCombine(m_inst->minecraftDir(), "bin/minecraft.jar")));
	
	
	
	////////////////////
	// DOWNLOAD FILES //
	////////////////////
	setState(StateDownloadFiles);
	for (int i = 0; i < m_downloadList.length(); i++)
	{
		m_currentDownload = i;
		if (!downloadFile(m_downloadList[i]))
			return;
	}
	
	
	
	///////////////////
	// INSTALL FILES //
	///////////////////
	setState(StateInstall);
	
	// Nothing to do here yet
	
	
	
	//////////////
	// FINISHED //
	//////////////
	setState(StateFinished);
	emit gameUpdateComplete(m_response);
}

bool GameUpdateTask::downloadFile( const FileToDownloadPtr file )
{
	setSubStatus("Downloading " + file->url().toString());
	QNetworkReply *reply = netMgr->get(QNetworkRequest(file->url()));
	
	this->connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
				  SLOT(updateDownloadProgress(qint64,qint64)));
	
	NetUtils::waitForNetRequest(reply);
	
	if (reply->error() == QNetworkReply::NoError)
	{
        QString filePath = file->path();
		QFile outFile(filePath);
		if (outFile.exists() && !outFile.remove())
		{
			error("Can't delete old file " + file->path() + ": " + outFile.errorString());
			return false;
		}
		
		if (!outFile.open(QIODevice::WriteOnly))
		{
			error("Can't write to " + file->path() + ": " + outFile.errorString());
			return false;
		}
		
		outFile.write(reply->readAll());
		outFile.close();
	}
	else
	{
		error("Can't download " + file->url().toString() + ": " + reply->errorString());
		return false;
	}
	
	// TODO: Check file integrity after downloading.
	
	return true;
}

int GameUpdateTask::state() const
{
	return m_updateState;
}

void GameUpdateTask::setState(int state, bool resetSubStatus)
{
	m_updateState = state;
	if (resetSubStatus)
		setSubStatus("");
	else // We only need to update if we're not resetting substatus becasue setSubStatus updates status for us.
		updateStatus();
}

QString GameUpdateTask::subStatus() const
{
	return m_subStatusMsg;
}

void GameUpdateTask::setSubStatus(const QString &msg)
{
	m_subStatusMsg = msg;
	updateStatus();
}

QString GameUpdateTask::getStateMessage(int state)
{
	switch (state)
	{
	case StateInit:
		return "Initializing";
		
	case StateDetermineURLs:
		return "Determining files to download";
		
	case StateDownloadFiles:
		return "Downloading files";
		
	case StateInstall:
		return "Installing";
		
	case StateFinished:
		return "Finished";
	
	default:
		return "Downloading instance files";
	}
}

void GameUpdateTask::updateStatus()
{
	QString newStatus;
	
	newStatus = getStateMessage(state());
	if (!subStatus().isEmpty())
		newStatus += ": " + subStatus();
	else
		newStatus += "...";
	
	setStatus(newStatus);
}


void GameUpdateTask::error(const QString &msg)
{
	emit gameUpdateError(msg);
}

void GameUpdateTask::updateDownloadProgress(qint64 current, qint64 total)
{
	// The progress on the current file is current / total
	float currentDLProgress = (float) current / (float) total; // Cast ALL the values!
	
	// The overall progress is (current progress + files downloaded) / total files to download
	float overallDLProgress = ((currentDLProgress + m_currentDownload) / (float) m_downloadList.length());
	
	// Multiply by 100 to make it a percentage.
	setProgress((int)(overallDLProgress * 100));
}

FileToDownloadPtr FileToDownload::Create(const QUrl &url, const QString &path, QObject *parent)
{
	return FileToDownloadPtr(new FileToDownload (url, path, parent));
}

FileToDownload::FileToDownload(const QUrl &url, const QString &path, QObject *parent) : 
	QObject(parent), m_dlURL(url), m_dlPath(path)
{
	
}
