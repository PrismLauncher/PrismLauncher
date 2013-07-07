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

GameUpdateTask::GameUpdateTask(const LoginResponse &response, Instance *inst, QObject *parent) :
	Task(parent), m_response(response)
{
	m_inst = inst;
	m_updateState = StateInit;
}

void GameUpdateTask::executeTask()
{
	updateStatus();
	
	// Get a pointer to the version object that corresponds to the instance's version.
	targetVersion = (MinecraftVersion *)MinecraftVersionList::getMainList().
			findVersion(m_inst->intendedVersion());
	if(targetVersion == NULL)
	{
		//Q_ASSERT_X(targetVersion != NULL, "game update", "instance's intended version is not an actual version");
		setState(StateFinished);
		emit gameUpdateComplete(m_response);
		return;
	}
	
	/////////////////////////
	// BUILD DOWNLOAD LIST //
	/////////////////////////
	// Build a list of URLs that will need to be downloaded.
	
	setState(StateDetermineURLs);
	
	if (targetVersion->launcherVersion() == MinecraftVersion::Launcher16)
	{
		determineNewVersion();
	}
	else
	{
		getLegacyJar();
	}
	QEventLoop loop;
	loop.exec();
}

void GameUpdateTask::determineNewVersion()
{
	QString urlstr("http://s3.amazonaws.com/Minecraft.Download/versions/");
	urlstr += targetVersion->descriptor() + "/" + targetVersion->descriptor() + ".json";
	auto dljob = DownloadJob::create(QUrl(urlstr));
	specificVersionDownloadJob.reset(new JobList());
	specificVersionDownloadJob->add(dljob);
	connect(specificVersionDownloadJob.data(), SIGNAL(finished()), SLOT(versionFileFinished()));
	connect(specificVersionDownloadJob.data(), SIGNAL(failed()), SLOT(versionFileFailed()));
	connect(specificVersionDownloadJob.data(), SIGNAL(progress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));
	download_queue.enqueue(specificVersionDownloadJob);
}

void GameUpdateTask::versionFileFinished()
{
	JobPtr firstJob = specificVersionDownloadJob->getFirstJob();
	auto DlJob = firstJob.dynamicCast<DownloadJob>();
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(DlJob->m_data, &jsonError);
	
	if (jsonError.error != QJsonParseError::NoError)
	{
		error(QString( "Error reading version file :") + " " + jsonError.errorString());
		exit(0);
	}
	
	Q_ASSERT_X(jsonDoc.isObject(), "loadFromVList", "jsonDoc is not an object");
	if(!jsonDoc.isObject())
	{
		error("Error reading version file.");
		exit(0);
	}
	QJsonObject root = jsonDoc.object();
	
	QString args = root.value("processArguments").toString("legacy");
	if(args == "legacy")
	{
		getLegacyJar();
		return;
	}
	
	
	error("MC 1.6 isn't supported yet...");
	exit(0);
}

void GameUpdateTask::versionFileFailed()
{
	error("Failed to download the version description. Try again.");
	exit(0);
}


// this is legacy minecraft...
void GameUpdateTask::getLegacyJar()
{
	// Make directories
	QDir binDir(m_inst->binDir());
	if (!binDir.exists() && !binDir.mkpath("."))
	{
		error("Failed to create bin folder.");
		return;
	}
	
	// Add the URL for minecraft.jar
	// This will be either 'minecraft' or the version number, depending on where
	// we're downloading from.
	QString jarFilename = "minecraft";
	if (targetVersion->launcherVersion() == MinecraftVersion::Launcher16)
	{
		jarFilename = targetVersion->descriptor();
	}
	
	QUrl mcJarURL = targetVersion->downloadURL() + jarFilename + ".jar";
	qDebug() << mcJarURL.toString();
	auto dljob = DownloadJob::create(mcJarURL, PathCombine(m_inst->minecraftDir(), "bin/minecraft.jar"));
	
	legacyDownloadJob.reset(new JobList());
	legacyDownloadJob->add(dljob);
	connect(legacyDownloadJob.data(), SIGNAL(finished()), SLOT(legacyJarFinished()));
	connect(legacyDownloadJob.data(), SIGNAL(failed()), SLOT(legacyJarFailed()));
	connect(legacyDownloadJob.data(), SIGNAL(progress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));
	
	download_queue.enqueue(legacyDownloadJob);
}


void GameUpdateTask::legacyJarFinished()
{
	setState(StateFinished);
	emit gameUpdateComplete(m_response);
	exit(1);
}

void GameUpdateTask::legacyJarFailed()
{
	emit gameUpdateError("failed to download the minecraft.jar");
	exit(0);
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
	float currentDLProgress = (float) current / (float) total;
	setProgress((int)(currentDLProgress * 100)); // convert to percentage
}

