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

#include "BaseInstance.h"
#include "lists/MinecraftVersionList.h"
#include "OneSixVersion.h"
#include "OneSixLibrary.h"
#include "OneSixInstance.h"
#include "net/ForgeMirrors.h"

#include "pathutils.h"

OneSixUpdate::OneSixUpdate(BaseInstance *inst, QObject *parent) : BaseUpdate(inst, parent)
{
}

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
	targetVersion = std::dynamic_pointer_cast<MinecraftVersion>(
		MMC->minecraftlist()->findVersion(intendedVersion));
	if (targetVersion == nullptr)
	{
		// don't do anything if it was invalid
		emitSucceeded();
		return;
	}

	if (m_inst->shouldUpdate())
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
	urlstr += targetVersion->descriptor() + "/" + targetVersion->descriptor() + ".json";
	auto job = new NetJob("Version index");
	job->addNetAction(ByteArrayDownload::make(QUrl(urlstr)));
	specificVersionDownloadJob.reset(job);
	connect(specificVersionDownloadJob.get(), SIGNAL(succeeded()), SLOT(versionFileFinished()));
	connect(specificVersionDownloadJob.get(), SIGNAL(failed()), SLOT(versionFileFailed()));
	connect(specificVersionDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));
	specificVersionDownloadJob->start();
}

void OneSixUpdate::versionFileFinished()
{
	NetActionPtr DlJob = specificVersionDownloadJob->first();
	OneSixInstance *inst = (OneSixInstance *)m_inst;

	QString version_id = targetVersion->descriptor();
	QString inst_dir = m_inst->instanceRoot();
	// save the version file in $instanceId/version.json
	{
		QString version1 = PathCombine(inst_dir, "/version.json");
		ensureFilePathExists(version1);
		// FIXME: detect errors here, download to a temp file, swap
		QSaveFile vfile1(version1);
		if (!vfile1.open(QIODevice::Truncate | QIODevice::WriteOnly))
		{
			emitFailed("Can't open " + version1 + " for writing.");
			return;
		}
		auto data = std::dynamic_pointer_cast<ByteArrayDownload>(DlJob)->m_data;
		qint64 actual = 0;
		if ((actual = vfile1.write(data)) != data.size())
		{
			emitFailed("Failed to write into " + version1 + ". Written " + actual + " out of " +
					   data.size() + '.');
			return;
		}
		if (!vfile1.commit())
		{
			emitFailed("Can't commit changes to " + version1);
			return;
		}
	}

	// the version is downloaded safely. update is 'done' at this point
	m_inst->setShouldUpdate(false);

	// delete any custom version inside the instance (it's no longer relevant, we did an update)
	QString custom = PathCombine(inst_dir, "/custom.json");
	QFile finfo(custom);
	if (finfo.exists())
	{
		finfo.remove();
	}
	inst->reloadFullVersion();

	jarlibStart();
}

void OneSixUpdate::versionFileFailed()
{
	emitFailed("Failed to download the version description. Try again.");
}

void OneSixUpdate::jarlibStart()
{
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	bool successful = inst->reloadFullVersion();
	if (!successful)
	{
		emitFailed("Failed to load the version description file (version.json). It might be "
				   "corrupted, missing or simply too new.");
		return;
	}

	std::shared_ptr<OneSixVersion> version = inst->getFullVersion();

	// download the right jar, save it in versions/$version/$version.jar
	QString urlstr("http://s3.amazonaws.com/Minecraft.Download/versions/");
	urlstr += version->id + "/" + version->id + ".jar";
	QString targetstr("versions/");
	targetstr += version->id + "/" + version->id + ".jar";

	auto job = new NetJob("Libraries for instance " + inst->name());
	job->addNetAction(FileDownload::make(QUrl(urlstr), targetstr));
	jarlibDownloadJob.reset(job);

	auto libs = version->getActiveNativeLibs();
	libs.append(version->getActiveNormalLibs());

	auto metacache = MMC->metacache();
	QList<ForgeXzDownloadPtr> ForgeLibs;
	bool already_forge_xz = false;
	for (auto lib : libs)
	{
		if (lib->hint() == "local")
			continue;
		auto entry = metacache->resolveEntry("libraries", lib->storagePath());
		if (entry->stale)
		{
			if (lib->hint() == "forge-pack-xz")
			{
				ForgeLibs.append(ForgeXzDownload::make(lib->storagePath(), entry));
			}
			else
			{
				jarlibDownloadJob->addNetAction(CacheDownload::make(lib->downloadUrl(), entry));
			}
		}
	}
	// TODO: think about how to propagate this from the original json file... or IF AT ALL
	QString forgeMirrorList = "http://files.minecraftforge.net/mirror-brand.list";
	if (!ForgeLibs.empty())
	{
		jarlibDownloadJob->addNetAction(
			ForgeMirrors::make(ForgeLibs, jarlibDownloadJob, forgeMirrorList));
	}

	connect(jarlibDownloadJob.get(), SIGNAL(succeeded()), SLOT(jarlibFinished()));
	connect(jarlibDownloadJob.get(), SIGNAL(failed()), SLOT(jarlibFailed()));
	connect(jarlibDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));

	jarlibDownloadJob->start();
}

void OneSixUpdate::jarlibFinished()
{
	emitSucceeded();
}

void OneSixUpdate::jarlibFailed()
{
	QStringList failed = jarlibDownloadJob->getFailedFiles();
	QString failed_all = failed.join("\n");
	emitFailed("Failed to download the following files:\n" + failed_all +
			   "\n\nPlease try again.");
}
