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

#include "MultiMC.h"
#include "OneSixUpdate.h"

#include <QtNetwork>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>
#include <pathutils.h>
#include <JlCompress.h>

#include "logic/BaseInstance.h"
#include "logic/minecraft/MinecraftVersionList.h"
#include "logic/minecraft/InstanceVersion.h"
#include "logic/minecraft/OneSixLibrary.h"
#include "logic/OneSixInstance.h"
#include "logic/forge/ForgeMirrors.h"
#include "logic/net/URLConstants.h"
#include "logic/assets/AssetsUtils.h"
#include "JarUtils.h"

OneSixUpdate::OneSixUpdate(OneSixInstance *inst, QObject *parent) : Task(parent), m_inst(inst)
{
}

void OneSixUpdate::executeTask()
{
	// Make directories
	QDir mcDir(m_inst->minecraftRoot());
	if (!mcDir.exists() && !mcDir.mkpath("."))
	{
		emitFailed(tr("Failed to create folder for minecraft binaries."));
		return;
	}

	// Get a pointer to the version object that corresponds to the instance's version.
	targetVersion = std::dynamic_pointer_cast<MinecraftVersion>(
		MMC->minecraftlist()->findVersion(m_inst->intendedVersionId()));
	if (targetVersion == nullptr)
	{
		// don't do anything if it was invalid
		emitFailed(tr("The specified Minecraft version is invalid. Choose a different one."));
		return;
	}
	if (m_inst->providesVersionFile() || !targetVersion->needsUpdate())
	{
		QLOG_DEBUG() << "Instance either provides a version file or doesn't need an update.";
		jarlibStart();
		return;
	}
	versionUpdateTask = MMC->minecraftlist()->createUpdateTask(m_inst->intendedVersionId());
	if (!versionUpdateTask)
	{
		QLOG_DEBUG() << "Didn't spawn an update task.";
		jarlibStart();
		return;
	}
	connect(versionUpdateTask.get(), SIGNAL(succeeded()), SLOT(jarlibStart()));
	connect(versionUpdateTask.get(), SIGNAL(failed(QString)), SLOT(versionUpdateFailed(QString)));
	connect(versionUpdateTask.get(), SIGNAL(progress(qint64, qint64)),
			SIGNAL(progress(qint64, qint64)));
	setStatus(tr("Getting the version files from Mojang..."));
	versionUpdateTask->start();
}

void OneSixUpdate::versionUpdateFailed(QString reason)
{
	emitFailed(reason);
}

void OneSixUpdate::assetIndexStart()
{
	setStatus(tr("Updating assets index..."));
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	std::shared_ptr<InstanceVersion> version = inst->getFullVersion();
	QString assetName = version->assets;
	QUrl indexUrl = "http://" + URLConstants::AWS_DOWNLOAD_INDEXES + assetName + ".json";
	QString localPath = assetName + ".json";
	auto job = new NetJob(tr("Asset index for %1").arg(inst->name()));

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
	std::shared_ptr<InstanceVersion> version = inst->getFullVersion();
	QString assetName = version->assets;

	QString asset_fname = "assets/indexes/" + assetName + ".json";
	if (!AssetsUtils::loadAssetsIndexJson(asset_fname, &index))
	{
		emitFailed(tr("Failed to read the assets index!"));
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
			objectDL->m_total_progress = object.size;
			dls.append(objectDL);
		}
	}
	if (dls.size())
	{
		setStatus(tr("Getting the assets files from Mojang..."));
		auto job = new NetJob(tr("Assets for %1").arg(inst->name()));
		for (auto dl : dls)
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
	emitFailed(tr("Failed to download the assets index!"));
}

void OneSixUpdate::assetsFinished()
{
	emitSucceeded();
}

void OneSixUpdate::assetsFailed()
{
	emitFailed(tr("Failed to download assets!"));
}

void OneSixUpdate::jarlibStart()
{
	setStatus(tr("Getting the library files from Mojang..."));
	QLOG_INFO() << m_inst->name() << ": downloading libraries";
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	try
	{
		inst->reloadVersion();
	}
	catch (MMCError &e)
	{
		emitFailed(e.cause());
		return;
	}
	catch (...)
	{
		emitFailed(tr("Failed to load the version description file for reasons unknown."));
		return;
	}

	// Build a list of URLs that will need to be downloaded.
	std::shared_ptr<InstanceVersion> version = inst->getFullVersion();
	// minecraft.jar for this version
	{
		QString version_id = version->id;
		QString localPath = version_id + "/" + version_id + ".jar";
		QString urlstr = "http://" + URLConstants::AWS_DOWNLOAD_VERSIONS + localPath;

		auto job = new NetJob(tr("Libraries for instance %1").arg(inst->name()));

		auto metacache = MMC->metacache();
		auto entry = metacache->resolveEntry("versions", localPath);
		job->addNetAction(CacheDownload::make(QUrl(urlstr), entry));
		jarHashOnEntry = entry->md5sum;

		jarlibDownloadJob.reset(job);
	}

	auto libs = version->getActiveNativeLibs();
	libs.append(version->getActiveNormalLibs());

	auto metacache = MMC->metacache();
	QList<ForgeXzDownloadPtr> ForgeLibs;
	QList<std::shared_ptr<OneSixLibrary>> brokenLocalLibs;

	for (auto lib : libs)
	{
		if (lib->hint() == "local")
		{
			if (!lib->filesExist(m_inst->librariesPath()))
				brokenLocalLibs.append(lib);
			continue;
		}

		QString raw_storage = lib->storagePath();
		QString raw_dl = lib->downloadUrl();

		auto f = [&](QString storage, QString dl)
		{
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
		};
		if (raw_storage.contains("${arch}"))
		{
			QString cooked_storage = raw_storage;
			QString cooked_dl = raw_dl;
			f(cooked_storage.replace("${arch}", "32"), cooked_dl.replace("${arch}", "32"));
			cooked_storage = raw_storage;
			cooked_dl = raw_dl;
			f(cooked_storage.replace("${arch}", "64"), cooked_dl.replace("${arch}", "64"));
		}
		else
		{
			f(raw_storage, raw_dl);
		}
	}
	if (!brokenLocalLibs.empty())
	{
		jarlibDownloadJob.reset();
		QStringList failed;
		for (auto brokenLib : brokenLocalLibs)
		{
			failed.append(brokenLib->files());
		}
		QString failed_all = failed.join("\n");
		emitFailed(tr("Some libraries marked as 'local' are missing their jar "
					  "files:\n%1\n\nYou'll have to correct this problem manually. If this is "
					  "an externally tracked instance, make sure to run it at least once "
					  "outside of MultiMC.").arg(failed_all));
		return;
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
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	std::shared_ptr<InstanceVersion> version = inst->getFullVersion();

	// nuke obsolete stripped jar(s) if needed
	QString version_id = version->id;
	QString strippedPath = version_id + "/" + version_id + "-stripped.jar";
	QFile strippedJar(strippedPath);
	if(strippedJar.exists())
	{
		strippedJar.remove();
	}
	auto finalJarPath = QDir(m_inst->instanceRoot()).absoluteFilePath("temp.jar");
	QFile finalJar(finalJarPath);
	if(finalJar.exists())
	{
		if(!finalJar.remove())
		{
			emitFailed(tr("Couldn't remove stale jar file: %1").arg(finalJarPath));
			return;
		}
	}

	// create stripped jar, if needed
	if (version->hasJarMods())
	{
		auto sourceJarPath = m_inst->versionsPath().absoluteFilePath(version->id + "/" + version->id + ".jar");
		QString localPath = version_id + "/" + version_id + ".jar";
		auto metacache = MMC->metacache();
		auto entry = metacache->resolveEntry("versions", localPath);
		QString fullJarPath = entry->getFullPath();
		//FIXME: remove need to convert to different objects here
		QList<Mod> mods;
		for (auto jarmod : version->jarMods)
		{
			QString filePath = m_inst->jarmodsPath().absoluteFilePath(jarmod->name);
			mods.push_back(Mod(QFileInfo(filePath)));
		}
		if(!JarUtils::createModdedJar(sourceJarPath, finalJarPath, mods))
		{
			emitFailed(tr("Failed to create the custom Minecraft jar file."));
			return;
		}
	}
	if (version->traits.contains("legacyFML"))
	{
		fmllibsStart();
	}
	else
	{
		assetIndexStart();
	}
}

void OneSixUpdate::jarlibFailed()
{
	QStringList failed = jarlibDownloadJob->getFailedFiles();
	QString failed_all = failed.join("\n");
	emitFailed(
		tr("Failed to download the following files:\n%1\n\nPlease try again.").arg(failed_all));
}

void OneSixUpdate::fmllibsStart()
{
	// Get the mod list
	OneSixInstance *inst = (OneSixInstance *)m_inst;
	std::shared_ptr<InstanceVersion> fullversion = inst->getFullVersion();
	bool forge_present = false;

	QString version = inst->intendedVersionId();
	auto &fmlLibsMapping = g_VersionFilterData.fmlLibsMapping;
	if (!fmlLibsMapping.contains(version))
	{
		assetIndexStart();
		return;
	}

	auto &libList = fmlLibsMapping[version];

	// determine if we need some libs for FML or forge
	setStatus(tr("Checking for FML libraries..."));
	forge_present = (fullversion->versionPatch("net.minecraftforge") != nullptr);
	// we don't...
	if (!forge_present)
	{
		assetIndexStart();
		return;
	}

	// now check the lib folder inside the instance for files.
	for (auto &lib : libList)
	{
		QFileInfo libInfo(PathCombine(inst->libDir(), lib.filename));
		if (libInfo.exists())
			continue;
		fmlLibsToProcess.append(lib);
	}

	// if everything is in place, there's nothing to do here...
	if (fmlLibsToProcess.isEmpty())
	{
		assetIndexStart();
		return;
	}

	// download missing libs to our place
	setStatus(tr("Dowloading FML libraries..."));
	auto dljob = new NetJob("FML libraries");
	auto metacache = MMC->metacache();
	for (auto &lib : fmlLibsToProcess)
	{
		auto entry = metacache->resolveEntry("fmllibs", lib.filename);
		QString urlString = lib.ours ? URLConstants::FMLLIBS_OUR_BASE_URL + lib.filename
									 : URLConstants::FMLLIBS_FORGE_BASE_URL + lib.filename;
		dljob->addNetAction(CacheDownload::make(QUrl(urlString), entry));
	}

	connect(dljob, SIGNAL(succeeded()), SLOT(fmllibsFinished()));
	connect(dljob, SIGNAL(failed()), SLOT(fmllibsFailed()));
	connect(dljob, SIGNAL(progress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	legacyDownloadJob.reset(dljob);
	legacyDownloadJob->start();
}

void OneSixUpdate::fmllibsFinished()
{
	legacyDownloadJob.reset();
	if (!fmlLibsToProcess.isEmpty())
	{
		setStatus(tr("Copying FML libraries into the instance..."));
		OneSixInstance *inst = (OneSixInstance *)m_inst;
		auto metacache = MMC->metacache();
		int index = 0;
		for (auto &lib : fmlLibsToProcess)
		{
			progress(index, fmlLibsToProcess.size());
			auto entry = metacache->resolveEntry("fmllibs", lib.filename);
			auto path = PathCombine(inst->libDir(), lib.filename);
			if (!ensureFilePathExists(path))
			{
				emitFailed(tr("Failed creating FML library folder inside the instance."));
				return;
			}
			if (!QFile::copy(entry->getFullPath(), PathCombine(inst->libDir(), lib.filename)))
			{
				emitFailed(tr("Failed copying Forge/FML library: %1.").arg(lib.filename));
				return;
			}
			index++;
		}
		progress(index, fmlLibsToProcess.size());
	}
	assetIndexStart();
}

void OneSixUpdate::fmllibsFailed()
{
	emitFailed("Game update failed: it was impossible to fetch the required FML libraries.");
	return;
}
