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
#include "MultiMC.h"
#include "OneSixUpdate.h"

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
#include "OneSixInstance.h"

#include "pathutils.h"


OneSixUpdate::OneSixUpdate(BaseInstance *inst, QObject *parent):BaseUpdate(inst, parent){}

void OneSixUpdate::executeTask()
{
	QString intendedVersion = m_inst->intendedVersionId();
	
	// Make directories
	QDir mcDir(m_inst->minecraftRoot());
	if (!mcDir.exists() && !mcDir.mkpath("."))
	{
		emitFailed("Failed to create bin folder.");
		return;
	}
	
	// Get a pointer to the version object that corresponds to the instance's version.
	targetVersion = MinecraftVersionList::getMainList().findVersion(intendedVersion).dynamicCast<MinecraftVersion>();
	if(targetVersion == nullptr)
	{
		// don't do anything if it was invalid
		emitSucceeded();
		return;
	}
	
	if(m_inst->shouldUpdate())
	{
		versionFileStart();
	}
	else
	{
		jarlibStart();
	}
}

void OneSixUpdate::versionFileStart()
{
	setStatus("Getting the version files from Mojang.");
	
	QString urlstr("http://s3.amazonaws.com/Minecraft.Download/versions/");
	urlstr += targetVersion->descriptor + "/" + targetVersion->descriptor + ".json";
	auto job = new DownloadJob("Version index");
	job->add(QUrl(urlstr));
	specificVersionDownloadJob.reset(job);
	connect(specificVersionDownloadJob.data(), SIGNAL(succeeded()), SLOT(versionFileFinished()));
	connect(specificVersionDownloadJob.data(), SIGNAL(failed()), SLOT(versionFileFailed()));
	connect(specificVersionDownloadJob.data(), SIGNAL(progress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));
	specificVersionDownloadJob->start();
}

void OneSixUpdate::versionFileFinished()
{
	DownloadPtr DlJob = specificVersionDownloadJob->first();
	
	QString version_id = targetVersion->descriptor;
	QString inst_dir = m_inst->instanceRoot();
	// save the version file in $instanceId/version.json
	{
		QString version1 =  PathCombine(inst_dir, "/version.json");
		ensureFilePathExists(version1);
		// FIXME: detect errors here, download to a temp file, swap
		QFile  vfile1 (version1);
		vfile1.open(QIODevice::Truncate | QIODevice::WriteOnly );
		vfile1.write(DlJob.dynamicCast<ByteArrayDownload>()->m_data);
		vfile1.close();
	}
	
	// the version is downloaded safely. update is 'done' at this point
	m_inst->setShouldUpdate(false);
	// save the version file in versions/$version/$version.json
	/*
		//QString version2 =  QString("versions/") + version_id + "/" + version_id + ".json";
		//ensurePathExists(version2);
		//QFile  vfile2 (version2);
		//vfile2.open(QIODevice::Truncate | QIODevice::WriteOnly );
		//vfile2.write(DlJob->m_data);
		//vfile2.close();
	*/
	
	jarlibStart();
}

void OneSixUpdate::versionFileFailed()
{
	emitFailed("Failed to download the version description. Try again.");
}

void OneSixUpdate::jarlibStart()
{
	OneSixInstance * inst = (OneSixInstance *) m_inst;
	bool successful = inst->reloadFullVersion();
	if(!successful)
	{
		emitFailed("Failed to load the version description file (version.json). It might be corrupted, missing or simply too new.");
		return;
	}
	
	QSharedPointer<OneSixVersion> version = inst->getFullVersion();
	
	// download the right jar, save it in versions/$version/$version.jar
	QString urlstr("http://s3.amazonaws.com/Minecraft.Download/versions/");
	urlstr += version->id + "/" + version->id + ".jar";
	QString targetstr ("versions/");
	targetstr += version->id + "/" + version->id + ".jar";
	
	auto job = new DownloadJob("Libraries for instance " + inst->name());
	job->add(QUrl(urlstr), targetstr);
	jarlibDownloadJob.reset(job);
	
	auto libs = version->getActiveNativeLibs();
	libs.append(version->getActiveNormalLibs());
	
	auto metacache = MMC->metacache();
	for(auto lib: libs)
	{
		QString download_path = lib->downloadPath();
		auto entry = metacache->resolveEntry("libraries", lib->storagePath());
		if(entry->stale)
		{
			jarlibDownloadJob->add(download_path, entry);
		}
	}
	connect(jarlibDownloadJob.data(), SIGNAL(succeeded()), SLOT(jarlibFinished()));
	connect(jarlibDownloadJob.data(), SIGNAL(failed()), SLOT(jarlibFailed()));
	connect(jarlibDownloadJob.data(), SIGNAL(progress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));

	jarlibDownloadJob->start();
}

void OneSixUpdate::jarlibFinished()
{
	emitSucceeded();
}

void OneSixUpdate::jarlibFailed()
{
	emitFailed("Failed to download the binary garbage. Try again. Maybe. IF YOU DARE");
}

