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

#include "LegacyUpdate.h"
#include "lists/LwjglVersionList.h"
#include "lists/MinecraftVersionList.h"
#include "BaseInstance.h"
#include "LegacyInstance.h"
#include "MultiMC.h"
#include "ModList.h"
#include <pathutils.h>
#include <quazip.h>
#include <quazipfile.h>
#include <JlCompress.h>
#include "logger/QsLog.h"

LegacyUpdate::LegacyUpdate(BaseInstance *inst, QObject *parent) : BaseUpdate(inst, parent)
{
}

void LegacyUpdate::executeTask()
{
	lwjglStart();
}

void LegacyUpdate::lwjglStart()
{
	LegacyInstance *inst = (LegacyInstance *)m_inst;

	lwjglVersion = inst->lwjglVersion();
	lwjglTargetPath = PathCombine(MMC->settings()->get("LWJGLDir").toString(), lwjglVersion);
	lwjglNativesPath = PathCombine(lwjglTargetPath, "natives");

	// if the 'done' file exists, we don't have to download this again
	QFileInfo doneFile(PathCombine(lwjglTargetPath, "done"));
	if (doneFile.exists())
	{
		jarStart();
		return;
	}

	auto list = MMC->lwjgllist();
	if (!list->isLoaded())
	{
		emitFailed("Too soon! Let the LWJGL list load :)");
		return;
	}

	setStatus("Downloading new LWJGL.");
	auto version = list->getVersion(lwjglVersion);
	if (!version)
	{
		emitFailed("Game update failed: the selected LWJGL version is invalid.");
		return;
	}

	QString url = version->url();
	QUrl realUrl(url);
	QString hostname = realUrl.host();
	auto worker = MMC->qnam();
	QNetworkRequest req(realUrl);
	req.setRawHeader("Host", hostname.toLatin1());
	req.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Cached)");
	QNetworkReply *rep = worker->get(req);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	connect(worker.get(), SIGNAL(finished(QNetworkReply *)),
			SLOT(lwjglFinished(QNetworkReply *)));
	// connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
	// SLOT(downloadError(QNetworkReply::NetworkError)));
}

void LegacyUpdate::lwjglFinished(QNetworkReply *reply)
{
	if (m_reply.get() != reply)
	{
		return;
	}
	if (reply->error() != QNetworkReply::NoError)
	{
		emitFailed("Failed to download: " + reply->errorString() +
				   "\nSometimes you have to wait a bit if you download many LWJGL versions in "
				   "a row. YMMV");
		return;
	}
	auto worker = MMC->qnam();
	// Here i check if there is a cookie for me in the reply and extract it
	QList<QNetworkCookie> cookies =
		qvariant_cast<QList<QNetworkCookie>>(reply->header(QNetworkRequest::SetCookieHeader));
	if (cookies.count() != 0)
	{
		// you must tell which cookie goes with which url
		worker->cookieJar()->setCookiesFromUrl(cookies, QUrl("sourceforge.net"));
	}

	// here you can check for the 302 or whatever other header i need
	QVariant newLoc = reply->header(QNetworkRequest::LocationHeader);
	if (newLoc.isValid())
	{
		QString redirectedTo = reply->header(QNetworkRequest::LocationHeader).toString();
		QUrl realUrl(redirectedTo);
		QString hostname = realUrl.host();
		QNetworkRequest req(redirectedTo);
		req.setRawHeader("Host", hostname.toLatin1());
		req.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Cached)");
		QNetworkReply *rep = worker->get(req);
		connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
				SIGNAL(progress(qint64, qint64)));
		m_reply = std::shared_ptr<QNetworkReply>(rep);
		return;
	}
	QFile saveMe("lwjgl.zip");
	saveMe.open(QIODevice::WriteOnly);
	saveMe.write(m_reply->readAll());
	saveMe.close();
	setStatus("Installing new LWJGL...");
	extractLwjgl();
	jarStart();
}
void LegacyUpdate::extractLwjgl()
{
	// make sure the directories are there

	bool success = ensureFolderPathExists(lwjglNativesPath);

	if (!success)
	{
		emitFailed("Failed to extract the lwjgl libs - error when creating required folders.");
		return;
	}

	QuaZip zip("lwjgl.zip");
	if (!zip.open(QuaZip::mdUnzip))
	{
		emitFailed("Failed to extract the lwjgl libs - not a valid archive.");
		return;
	}

	// and now we are going to access files inside it
	QuaZipFile file(&zip);
	const QString jarNames[] = {"jinput.jar", "lwjgl_util.jar", "lwjgl.jar"};
	for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
	{
		if (!file.open(QIODevice::ReadOnly))
		{
			zip.close();
			emitFailed("Failed to extract the lwjgl libs - error while reading archive.");
			return;
		}
		QuaZipFileInfo info;
		QString name = file.getActualFileName();
		if (name.endsWith('/'))
		{
			file.close();
			continue;
		}
		QString destFileName;
		// Look for the jars
		for (int i = 0; i < 3; i++)
		{
			if (name.endsWith(jarNames[i]))
			{
				destFileName = PathCombine(lwjglTargetPath, jarNames[i]);
			}
		}
		// Not found? look for the natives
		if (destFileName.isEmpty())
		{
#ifdef Q_OS_WIN32
			QString nativesDir = "windows";
#else
#ifdef Q_OS_MAC
			QString nativesDir = "macosx";
#else
			QString nativesDir = "linux";
#endif
#endif
			if (name.contains(nativesDir))
			{
				int lastSlash = name.lastIndexOf('/');
				int lastBackSlash = name.lastIndexOf('\\');
				if (lastSlash != -1)
					name = name.mid(lastSlash + 1);
				else if (lastBackSlash != -1)
					name = name.mid(lastBackSlash + 1);
				destFileName = PathCombine(lwjglNativesPath, name);
			}
		}
		// Now if destFileName is still empty, go to the next file.
		if (!destFileName.isEmpty())
		{
			setStatus("Installing new LWJGL - Extracting " + name);
			QFile output(destFileName);
			output.open(QIODevice::WriteOnly);
			output.write(file.readAll()); // FIXME: wste of memory!?
			output.close();
		}
		file.close(); // do not forget to close!
	}
	zip.close();
	m_reply.reset();
	QFile doneFile(PathCombine(lwjglTargetPath, "done"));
	doneFile.open(QIODevice::WriteOnly);
	doneFile.write("done.");
	doneFile.close();
}

void LegacyUpdate::lwjglFailed()
{
	emitFailed("Bad stuff happened while trying to get the lwjgl libs...");
}

void LegacyUpdate::jarStart()
{
	LegacyInstance *inst = (LegacyInstance *)m_inst;
	if (!inst->shouldUpdate() || inst->shouldUseCustomBaseJar())
	{
		ModTheJar();
		return;
	}

	setStatus("Checking for jar updates...");
	// Make directories
	QDir binDir(inst->binDir());
	if (!binDir.exists() && !binDir.mkpath("."))
	{
		emitFailed("Failed to create bin folder.");
		return;
	}

	// Build a list of URLs that will need to be downloaded.
	setStatus("Downloading new minecraft.jar");

	QString urlstr("http://s3.amazonaws.com/Minecraft.Download/versions/");
	QString intended_version_id = inst->intendedVersionId();
	urlstr += intended_version_id + "/" + intended_version_id + ".jar";

	auto dljob = new NetJob("Minecraft.jar for version " + intended_version_id);
	dljob->addNetAction(FileDownload::make(QUrl(urlstr), inst->defaultBaseJar()));
	legacyDownloadJob.reset(dljob);
	connect(dljob, SIGNAL(succeeded()), SLOT(jarFinished()));
	connect(dljob, SIGNAL(failed()), SLOT(jarFailed()));
	connect(dljob, SIGNAL(progress(qint64, qint64)), SIGNAL(progress(qint64, qint64)));
	legacyDownloadJob->start();
}

void LegacyUpdate::jarFinished()
{
	// process the jar
	ModTheJar();
}

void LegacyUpdate::jarFailed()
{
	// bad, bad
	emitFailed("Failed to download the minecraft jar. Try again later.");
}

bool LegacyUpdate::MergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained,
								 MetainfAction metainf)
{
	setStatus("Installing mods - Adding " + from.fileName());

	QuaZip modZip(from.filePath());
	modZip.open(QuaZip::mdUnzip);

	QuaZipFile fileInsideMod(&modZip);
	QuaZipFile zipOutFile(into);
	for (bool more = modZip.goToFirstFile(); more; more = modZip.goToNextFile())
	{
		QString filename = modZip.getCurrentFileName();
		if (filename.contains("META-INF") && metainf == LegacyUpdate::IgnoreMetainf)
		{
			QLOG_INFO() << "Skipping META-INF " << filename << " from " << from.fileName();
			continue;
		}
		if (contained.contains(filename))
		{
			QLOG_INFO() << "Skipping already contained file " << filename << " from "
						<< from.fileName();
			continue;
		}
		contained.insert(filename);
		QLOG_INFO() << "Adding file " << filename << " from " << from.fileName();

		if (!fileInsideMod.open(QIODevice::ReadOnly))
		{
			QLOG_ERROR() << "Failed to open " << filename << " from " << from.fileName();
			return false;
		}
		/*
		QuaZipFileInfo old_info;
		fileInsideMod.getFileInfo(&old_info);
		*/
		QuaZipNewInfo info_out(fileInsideMod.getActualFileName());
		/*
		info_out.externalAttr = old_info.externalAttr;
		*/
		if (!zipOutFile.open(QIODevice::WriteOnly, info_out))
		{
			QLOG_ERROR() << "Failed to open " << filename << " in the jar";
			fileInsideMod.close();
			return false;
		}
		if (!JlCompress::copyData(fileInsideMod, zipOutFile))
		{
			zipOutFile.close();
			fileInsideMod.close();
			QLOG_ERROR() << "Failed to copy data of " << filename << " into the jar";
			return false;
		}
		zipOutFile.close();
		fileInsideMod.close();
	}
	return true;
}

void LegacyUpdate::ModTheJar()
{
	LegacyInstance *inst = (LegacyInstance *)m_inst;

	if (!inst->shouldRebuild())
	{
		emitSucceeded();
		return;
	}

	// Get the mod list
	auto modList = inst->jarModList();

	QFileInfo runnableJar(inst->runnableJar());
	QFileInfo baseJar(inst->baseJar());
	bool base_is_custom = inst->shouldUseCustomBaseJar();

	// Nothing to do if there are no jar mods to install, no backup and just the mc jar
	if (base_is_custom)
	{
		// yes, this can happen if the instance only has the runnable jar and not the base jar
		// it *could* be assumed that such an instance is vanilla, but that wouldn't be safe
		// because that's not something mmc4 guarantees
		if (runnableJar.isFile() && !baseJar.exists() && modList->empty())
		{
			inst->setShouldRebuild(false);
			emitSucceeded();
			return;
		}

		setStatus("Installing mods - backing up minecraft.jar...");
		if (!baseJar.exists() && !QFile::copy(runnableJar.filePath(), baseJar.filePath()))
		{
			emitFailed("It seems both the active and base jar are gone. A fresh base jar will be used on next run.");
			inst->setShouldRebuild(true);
			inst->setShouldUpdate(true);
			inst->setShouldUseCustomBaseJar(false);
			return;
		}
	}

	if (!baseJar.exists())
	{
		emitFailed("The base jar " + baseJar.filePath() + " does not exist");
		return;
	}

	if (runnableJar.exists() && !QFile::remove(runnableJar.filePath()))
	{
		emitFailed("Failed to delete old minecraft.jar");
		return;
	}

	// TaskStep(); // STEP 1
	setStatus("Installing mods - Opening minecraft.jar");

	QuaZip zipOut(runnableJar.filePath());
	if (!zipOut.open(QuaZip::mdCreate))
	{
		QFile::remove(runnableJar.filePath());
		emitFailed("Failed to open the minecraft.jar for modding");
		return;
	}
	// Files already added to the jar.
	// These files will be skipped.
	QSet<QString> addedFiles;

	// Modify the jar
	setStatus("Installing mods - Adding mod files...");
	for (int i = modList->size() - 1; i >= 0; i--)
	{
		auto &mod = modList->operator[](i);
		if (mod.type() == Mod::MOD_ZIPFILE)
		{
			if (!MergeZipFiles(&zipOut, mod.filename(), addedFiles, LegacyUpdate::KeepMetainf))
			{
				zipOut.close();
				QFile::remove(runnableJar.filePath());
				emitFailed("Failed to add " + mod.filename().fileName() + " to the jar.");
				return;
			}
		}
		else if (mod.type() == Mod::MOD_SINGLEFILE)
		{
			auto filename = mod.filename();
			if (!JlCompress::compressFile(&zipOut, filename.absoluteFilePath(),
										  filename.fileName()))
			{
				zipOut.close();
				QFile::remove(runnableJar.filePath());
				emitFailed("Failed to add " + filename.fileName() + " to the jar");
				return;
			}
			addedFiles.insert(filename.fileName());
			QLOG_INFO() << "Adding file " << filename.fileName() << " from "
						<< filename.absoluteFilePath();
		}
		else if (mod.type() == Mod::MOD_FOLDER)
		{
			auto filename = mod.filename();
			QString what_to_zip = filename.absoluteFilePath();
			QDir dir(what_to_zip);
			dir.cdUp();
			QString parent_dir = dir.absolutePath();
			if (!JlCompress::compressSubDir(&zipOut, what_to_zip, parent_dir, true, addedFiles))
			{
				zipOut.close();
				QFile::remove(runnableJar.filePath());
				emitFailed("Failed to add " + filename.fileName() + " to the jar");
				return;
			}
			QLOG_INFO() << "Adding folder " << filename.fileName() << " from "
						<< filename.absoluteFilePath();
		}
	}

	if (!MergeZipFiles(&zipOut, baseJar, addedFiles, LegacyUpdate::IgnoreMetainf))
	{
		zipOut.close();
		QFile::remove(runnableJar.filePath());
		emitFailed("Failed to insert minecraft.jar contents.");
		return;
	}

	// Recompress the jar
	zipOut.close();
	if (zipOut.getZipError() != 0)
	{
		QFile::remove(runnableJar.filePath());
		emitFailed("Failed to finalize minecraft.jar!");
		return;
	}
	inst->setShouldRebuild(false);
	// inst->UpdateVersion(true);
	emitSucceeded();
	return;
}