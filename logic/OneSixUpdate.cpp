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
#include "net/URLConstants.h"
#include "assets/AssetsUtils.h"

#include "pathutils.h"
#include <JlCompress.h>

OneSixUpdate::OneSixUpdate(BaseInstance *inst, bool only_prepare, QObject *parent)
	: Task(parent), m_inst(inst), m_only_prepare(only_prepare)
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

	if (m_only_prepare)
	{
		if (m_inst->shouldUpdate())
		{
			emitFailed("Unable to update instance in offline mode.");
			return;
		}
		setStatus("Testing the Java installation.");
		QString java_path = m_inst->settings().get("JavaPath").toString();

		checker.reset(new JavaChecker());
		connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
				SLOT(checkFinishedOffline(JavaCheckResult)));
		checker->path = java_path;
		checker->performCheck();
		return;
	}

	if (m_inst->shouldUpdate())
	{
		// Get a pointer to the version object that corresponds to the instance's version.
		targetVersion = std::dynamic_pointer_cast<MinecraftVersion>(
			MMC->minecraftlist()->findVersion(intendedVersion));
		if (targetVersion == nullptr)
		{
			// don't do anything if it was invalid
			emitFailed("The specified Minecraft version is invalid. Choose a different one.");
			return;
		}
		versionFileStart();
	}
	else
	{
		checkJavaOnline();
	}
}

void OneSixUpdate::checkJavaOnline()
{
	setStatus("Testing the Java installation.");
	QString java_path = m_inst->settings().get("JavaPath").toString();

	checker.reset(new JavaChecker());
	connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
			SLOT(checkFinishedOnline(JavaCheckResult)));
	checker->path = java_path;
	checker->performCheck();
}

void OneSixUpdate::checkFinishedOnline(JavaCheckResult result)
{
	if (result.valid)
	{
		java_is_64bit = result.is_64bit;
		jarlibStart();
	}
	else
	{
		emitFailed("The java binary doesn't work. Check the settings and correct the problem");
	}
}

void OneSixUpdate::checkFinishedOffline(JavaCheckResult result)
{
	if (result.valid)
	{
		java_is_64bit = result.is_64bit;
		prepareForLaunch();
	}
	else
	{
		emitFailed("The java binary doesn't work. Check the settings and correct the problem");
	}
}

void OneSixUpdate::versionFileStart()
{
	QLOG_INFO() << m_inst->name() << ": getting version file.";
	setStatus("Getting the version files from Mojang.");

	QString urlstr = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + targetVersion->descriptor() + "/" + targetVersion->descriptor() + ".json";
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

	checkJavaOnline();
}

void OneSixUpdate::versionFileFailed()
{
	emitFailed("Failed to download the version description. Try again.");
}

void OneSixUpdate::assetIndexStart()
{
	setStatus("Updating asset index.");
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	std::shared_ptr<OneSixVersion> version = inst->getFullVersion();
	QString assetName = version->assets;
	QUrl indexUrl = "http://" + URLConstants::AWS_DOWNLOAD_INDEXES + assetName + ".json";
	QString localPath = assetName + ".json";
	auto job = new NetJob("Asset index for " + inst->name());

	auto metacache = MMC->metacache();
	auto entry = metacache->resolveEntry("asset_indexes", localPath);
	job->addNetAction(CacheDownload::make(indexUrl, entry));
	jarlibDownloadJob.reset(job);

	connect(jarlibDownloadJob.get(), SIGNAL(succeeded()), SLOT(assetIndexFinished()));
	connect(jarlibDownloadJob.get(), SIGNAL(failed()), SLOT(assetIndexFailed()));
	connect(jarlibDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));

	jarlibDownloadJob->start();
}

void OneSixUpdate::assetIndexFinished()
{
	AssetsIndex index;

	OneSixInstance *inst = (OneSixInstance *)m_inst;
	std::shared_ptr<OneSixVersion> version = inst->getFullVersion();
	QString assetName = version->assets;

	QString asset_fname = "assets/indexes/" + assetName + ".json";
	if (!AssetsUtils::loadAssetsIndexJson(asset_fname, &index))
	{
		emitFailed("Failed to read the assets index!");
	}
	
	QList<Md5EtagDownloadPtr> dls;
	for (auto object : index.objects.values())
	{
		QString objectName = object.hash.left(2) + "/" + object.hash;
		QFileInfo objectFile("assets/objects/" + objectName);
		if ((!objectFile.isFile()) || (objectFile.size() != object.size))
		{
			auto objectDL = MD5EtagDownload::make(
				QUrl("http://" + URLConstants::RESOURCE_BASE + objectName),
				objectFile.filePath());
			dls.append(objectDL);
		}
	}
	if(dls.size())
	{
		auto job = new NetJob("Assets for " + inst->name());
		for(auto dl: dls)
			job->addNetAction(dl);
		jarlibDownloadJob.reset(job);
		connect(jarlibDownloadJob.get(), SIGNAL(succeeded()), SLOT(assetsFinished()));
		connect(jarlibDownloadJob.get(), SIGNAL(failed()), SLOT(assetsFailed()));
		connect(jarlibDownloadJob.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));
		jarlibDownloadJob->start();
		return;
	}
	assetsFinished();
}

void OneSixUpdate::assetIndexFailed()
{
	emitFailed("Failed to download the assets index!");
}

void OneSixUpdate::assetsFinished()
{
	prepareForLaunch();
}

void OneSixUpdate::assetsFailed()
{
	emitFailed("Failed to download assets!");
}



void OneSixUpdate::jarlibStart()
{
	setStatus("Getting the library files from Mojang.");
	QLOG_INFO() << m_inst->name() << ": downloading libraries";
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	bool successful = inst->reloadFullVersion();
	if (!successful)
	{
		emitFailed("Failed to load the version description file. It might be "
				   "corrupted, missing or simply too new.");
		return;
	}

	// Build a list of URLs that will need to be downloaded.
	std::shared_ptr<OneSixVersion> version = inst->getFullVersion();
	// minecraft.jar for this version
	{
		QString version_id = version->id;
		QString localPath = version_id + "/" + version_id + ".jar";
		QString urlstr = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + localPath;

		auto job = new NetJob("Libraries for instance " + inst->name());

		auto metacache = MMC->metacache();
		auto entry = metacache->resolveEntry("versions", localPath);
		job->addNetAction(CacheDownload::make(QUrl(urlstr), entry));

		jarlibDownloadJob.reset(job);
	}

	auto libs = version->getActiveNativeLibs();
	libs.append(version->getActiveNormalLibs());

	auto metacache = MMC->metacache();
	QList<ForgeXzDownloadPtr> ForgeLibs;
	for (auto lib : libs)
	{
		if (lib->hint() == "local")
			continue;
		QString subst = java_is_64bit ? "64" : "32";
		QString storage = lib->storagePath();
		QString dl = lib->downloadUrl();

		storage.replace("${arch}", subst);
		dl.replace("${arch}", subst);

		auto entry = metacache->resolveEntry("libraries", storage);
		if (entry->stale)
		{
			if (lib->hint() == "forge-pack-xz")
			{
				ForgeLibs.append(ForgeXzDownload::make(storage, entry));
			}
			else
			{
				jarlibDownloadJob->addNetAction(CacheDownload::make(dl, entry));
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
	assetIndexStart();
}

void OneSixUpdate::jarlibFailed()
{
	QStringList failed = jarlibDownloadJob->getFailedFiles();
	QString failed_all = failed.join("\n");
	emitFailed("Failed to download the following files:\n" + failed_all +
			   "\n\nPlease try again.");
}

void OneSixUpdate::prepareForLaunch()
{
	setStatus("Preparing for launch.");
	QLOG_INFO() << m_inst->name() << ": preparing for launch";
	auto onesix_inst = (OneSixInstance *)m_inst;

	// delete any leftovers, if they are present.
	onesix_inst->cleanupAfterRun();

	// Acquire swag
	QString natives_dir_raw = PathCombine(onesix_inst->instanceRoot(), "natives/");
	auto version = onesix_inst->getFullVersion();
	if (!version)
	{
		emitFailed("The version information for this instance is not complete. Try re-creating "
				   "it or changing the version.");
		return;
	}
	auto libs_to_extract = version->getActiveNativeLibs();

	// Acquire bag
	bool success = ensureFolderPathExists(natives_dir_raw);
	if (!success)
	{
		emitFailed("Could not create the native library folder:\n" + natives_dir_raw +
				   "\nMake sure MultiMC has appropriate permissions and there is enough space "
				   "on the storage device.");
		return;
	}

	// Put swag in the bag
	QString subst = java_is_64bit ? "64" : "32";
	for (auto lib : libs_to_extract)
	{
		QString storage = lib->storagePath();
		storage.replace("${arch}", subst);

		QString path = "libraries/" + storage;
		QLOG_INFO() << "Will extract " << path.toLocal8Bit();
		if (JlCompress::extractWithExceptions(path, natives_dir_raw, lib->extract_excludes)
				.isEmpty())
		{
			emitFailed(
				"Could not extract the native library:\n" + path +
				"\nMake sure MultiMC has appropriate permissions and there is enough space "
				"on the storage device.");
			return;
		}
	}

	// Show them your war face!
	emitSucceeded();
}
