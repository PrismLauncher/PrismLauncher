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

#include "GameUpdateTask.h"

#include <QtNetwork>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>

#include <QDebug>

#include "BaseInstance.h"
#include "lists/MinecraftVersionList.h"
#include "VersionFactory.h"
#include "OneSixVersion.h"

#include "pathutils.h"


GameUpdateTask::GameUpdateTask(BaseInstance *inst, QObject *parent) :
	Task(parent)
{
	m_inst = inst;
}

void GameUpdateTask::executeTask()
{
	// Get a pointer to the version object that corresponds to the instance's version.
	targetVersion = (MinecraftVersion *)MinecraftVersionList::getMainList().findVersion(m_inst->intendedVersionId());
	if(targetVersion == NULL)
	{
		emit gameUpdateComplete();
		return;
	}
	
	setStatus("Getting the version files from Mojang.");
	
	QString urlstr("http://s3.amazonaws.com/Minecraft.Download/versions/");
	urlstr += targetVersion->descriptor() + "/" + targetVersion->descriptor() + ".json";
	auto dljob = DownloadJob::create(QUrl(urlstr));
	specificVersionDownloadJob.reset(new JobList());
	specificVersionDownloadJob->add(dljob);
	connect(specificVersionDownloadJob.data(), SIGNAL(finished()), SLOT(versionFileFinished()));
	connect(specificVersionDownloadJob.data(), SIGNAL(failed()), SLOT(versionFileFailed()));
	connect(specificVersionDownloadJob.data(), SIGNAL(progress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));
	download_queue.enqueue(specificVersionDownloadJob);
	
	QEventLoop loop;
	loop.exec();
}

void GameUpdateTask::versionFileFinished()
{
	JobPtr firstJob = specificVersionDownloadJob->getFirstJob();
	auto DlJob = firstJob.dynamicCast<DownloadJob>();
	FullVersionFactory parser;
	auto version = parser.parse(DlJob->m_data);
	
	if(!version)
	{
		error(parser.error_string);
		exit(0);
	}
	
	// save the version file in $instanceId/version.json and versions/$version/$version.json
	QString version_id = targetVersion->descriptor();
	QString inst_dir = m_inst->rootDir();
	QString version1 =  PathCombine(inst_dir, "/version.json");
	QString version2 =  QString("versions/") + version_id + "/" + version_id + ".json";
	DownloadJob::ensurePathExists(version1);
	DownloadJob::ensurePathExists(version2);
	QFile  vfile1 (version1);
	QFile  vfile2 (version2);
	vfile1.open(QIODevice::Truncate | QIODevice::WriteOnly );
	vfile2.open(QIODevice::Truncate | QIODevice::WriteOnly );
	vfile1.write(DlJob->m_data);
	vfile2.write(DlJob->m_data);
	vfile1.close();
	vfile2.close();
	
	// download the right jar, save it in versions/$version/$version.jar
	QString urlstr("http://s3.amazonaws.com/Minecraft.Download/versions/");
	urlstr += targetVersion->descriptor() + "/" + targetVersion->descriptor() + ".jar";
	QString targetstr ("versions/");
	targetstr += targetVersion->descriptor() + "/" + targetVersion->descriptor() + ".jar";
	auto dljob = DownloadJob::create(QUrl(urlstr), targetstr);
	
	jarlibDownloadJob.reset(new JobList());
	jarlibDownloadJob->add(dljob);
	connect(jarlibDownloadJob.data(), SIGNAL(finished()), SLOT(jarlibFinished()));
	connect(jarlibDownloadJob.data(), SIGNAL(failed()), SLOT(jarlibFailed()));
	connect(jarlibDownloadJob.data(), SIGNAL(progress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));
	// determine and download all the libraries, save them in libraries/whatever...
	download_queue.enqueue(jarlibDownloadJob);
}

void GameUpdateTask::jarlibFinished()
{
	exit(1);
}

void GameUpdateTask::jarlibFailed()
{
	error("Failed to download the binary garbage. Try again. Maybe. IF YOU DARE");
	exit(0);
}

void GameUpdateTask::versionFileFailed()
{
	error("Failed to download the version description. Try again.");
	exit(0);
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

