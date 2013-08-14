#include "LegacyUpdate.h"
#include "lists/LwjglVersionList.h"
#include "lists/MinecraftVersionList.h"
#include "BaseInstance.h"
#include "LegacyInstance.h"
#include "net/NetWorker.h"
#include <pathutils.h>
#include <quazip.h>
#include <quazipfile.h>


LegacyUpdate::LegacyUpdate ( BaseInstance* inst, QObject* parent ) : BaseUpdate ( inst, parent ) {}

void LegacyUpdate::executeTask()
{
	lwjglStart();
}

void LegacyUpdate::lwjglStart()
{
	LegacyInstance * inst = (LegacyInstance *) m_inst;

	lwjglVersion =  inst->lwjglVersion();
	lwjglTargetPath = PathCombine("lwjgl", lwjglVersion );
	lwjglNativesPath = PathCombine( lwjglTargetPath, "natives/");
	
	// if the 'done' file exists, we don't have to download this again
	QFileInfo doneFile(PathCombine(lwjglTargetPath, "done"));
	if(doneFile.exists())
	{
		jarStart();
		return;
	}
	
	auto &list = LWJGLVersionList::get();
	if(!list.isLoaded())
	{
		emitFailed("Too soon! Let the LWJGL list load :)");
		return;
	}
	
	setStatus("Downloading new LWJGL.");
	auto version = list.getVersion(lwjglVersion);
	if(!version)
	{
		emitFailed("Game update failed: the selected LWJGL version is invalid.");
		return;
	}
	
	QString url = version->url();
	QUrl realUrl(url);
	QString hostname = realUrl.host();
	auto &worker = NetWorker::spawn();
	QNetworkRequest req(realUrl);
	req.setRawHeader("Host", hostname.toLatin1());
	req.setHeader(QNetworkRequest::UserAgentHeader, "Wget/1.14 (linux-gnu)");
	QNetworkReply * rep = worker.get ( req );
	
	m_reply = QSharedPointer<QNetworkReply> (rep, &QObject::deleteLater);
	connect(rep, SIGNAL(downloadProgress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));
	connect(&worker, SIGNAL(finished(QNetworkReply*)), SLOT(lwjglFinished(QNetworkReply*)));
	//connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
}

void LegacyUpdate::lwjglFinished(QNetworkReply* reply)
{
	if(m_reply != reply)
	{
		return;
	}
	if(reply->error() != QNetworkReply::NoError)
	{
		emitFailed( "Failed to download: "+
					reply->errorString()+
					"\nSometimes you have to wait a bit if you download many LWJGL versions in a row. YMMV");
		return;
	}
	auto &worker = NetWorker::spawn();
	//Here i check if there is a cookie for me in the reply and extract it
	QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie>>(reply->header(QNetworkRequest::SetCookieHeader));
	if(cookies.count() != 0)
	{
		//you must tell which cookie goes with which url
		worker.cookieJar()->setCookiesFromUrl(cookies, QUrl("sourceforge.net"));
	}

	//here you can check for the 302 or whatever other header i need
	QVariant newLoc = reply->header(QNetworkRequest::LocationHeader);
	if(newLoc.isValid())
	{
		auto &worker = NetWorker::spawn();
		QString redirectedTo = reply->header(QNetworkRequest::LocationHeader).toString();
		QUrl realUrl(redirectedTo);
		QString hostname = realUrl.host();
		QNetworkRequest req(redirectedTo);
		req.setRawHeader("Host", hostname.toLatin1());
		req.setHeader(QNetworkRequest::UserAgentHeader, "Wget/1.14 (linux-gnu)");
		QNetworkReply * rep = worker.get(req);
		connect(rep, SIGNAL(downloadProgress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));
		m_reply = QSharedPointer<QNetworkReply> (rep, &QObject::deleteLater);
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

	bool success = ensurePathExists(lwjglNativesPath);
	
	if(!success)
	{
		emitFailed("Failed to extract the lwjgl libs - error when creating required folders.");
		return;
	}
	
	QuaZip zip("lwjgl.zip");
	if(!zip.open(QuaZip::mdUnzip))
	{
		emitFailed("Failed to extract the lwjgl libs - not a valid archive.");
		return;
	}
	
	// and now we are going to access files inside it
	QuaZipFile file(&zip);
	const QString jarNames[] = { "jinput.jar", "lwjgl_util.jar", "lwjgl.jar" };
	for(bool more=zip.goToFirstFile(); more; more=zip.goToNextFile())
	{
		if(!file.open(QIODevice::ReadOnly))
		{
			zip.close();
			emitFailed("Failed to extract the lwjgl libs - error while reading archive.");
			return;
		}
		QuaZipFileInfo info;
		QString name = file.getActualFileName();
		if(name.endsWith('/'))
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
		if(destFileName.isEmpty())
		{
#ifdef Q_OS_WIN32
			QString nativesDir = "windows";
#elif Q_OS_MAC
			QString nativesDir = "macosx";
#else
			QString nativesDir = "linux";
#endif
			if (name.contains(nativesDir))
			{
				int lastSlash = name.lastIndexOf('/');
				int lastBackSlash = name.lastIndexOf('/');
				if(lastSlash != -1)
					name = name.mid(lastSlash+1);
				else if(lastBackSlash != -1)
					name = name.mid(lastBackSlash+1);
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
	m_reply.clear();
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
	setStatus("Checking ...");
	LegacyInstance * inst = (LegacyInstance *) m_inst;
	QString current_version_id = inst->currentVersionId();
	QString intended_version_id = inst->intendedVersionId();
	bool shouldUpdate = inst->shouldUpdate();
	if(!shouldUpdate)
	{
		emitSucceeded();
		return;
	}
	
	// nuke the backup file, we are replacing the base jar anyway
	QFile mc_backup(inst->mcBackup());
	if (mc_backup.exists())
	{
		mc_backup.remove();
	}
	
	// Get a pointer to the version object that corresponds to the instance's version.
	auto targetVersion = MinecraftVersionList::getMainList().findVersion(intended_version_id);
	
	if(!targetVersion)
	{
		emitFailed("Not a valid version:" + intended_version_id);
		return;
	}

	// Make directories
	QDir binDir(inst->binDir());
	if (!binDir.exists() && !binDir.mkpath("."))
	{
		emitFailed("Failed to create bin folder.");
		return;
	}

	// Build a list of URLs that will need to be downloaded.
	setStatus("Downloading new minecraft.jar");

	// This will be either 'minecraft' or the version number, depending on where
	// we're downloading from.
	QString jarFilename = "minecraft";
	QString download_path = PathCombine(inst->minecraftRoot(), "bin/minecraft.jar");

	QString urlstr("http://s3.amazonaws.com/Minecraft.Download/versions/");
	urlstr += targetVersion->descriptor + "/" + targetVersion->descriptor + ".jar";
	auto dljob = DownloadJob::create(QUrl(urlstr), download_path);
	
	legacyDownloadJob.reset(new JobList());
	legacyDownloadJob->add(dljob);
	connect(legacyDownloadJob.data(), SIGNAL(finished()), SLOT(jarFinished()));
	connect(legacyDownloadJob.data(), SIGNAL(failed()), SLOT(jarFailed()));
	connect(legacyDownloadJob.data(), SIGNAL(progress(qint64,qint64)), SLOT(updateDownloadProgress(qint64,qint64)));
	download_queue.enqueue(legacyDownloadJob);
}

void LegacyUpdate::jarFinished()
{
	// process the jar
	emitSucceeded();
}

void LegacyUpdate::jarFailed()
{
	// bad, bad
	emitFailed("Failed to download the minecraft jar. Try again later.");
}

void LegacyUpdate::ModTheJar()
{
	/*
	LegacyInstance * inst = (LegacyInstance *) m_inst;
	// Get the mod list
	auto modList = inst->getJarModList();
	
	QFileInfo mcBin(inst->binDir());
	QFileInfo mcJar(inst->mcJar());
	QFileInfo mcBackup(inst->mcBackup());
	
	// Nothing to do if there are no jar mods to install, no backup and just the mc jar
	if(mcJar.isFile() && !mcBackup.exists() && modList->empty())
	{
		inst->setShouldRebuild(false);
		emitSucceeded();
		return;
	}
	
	setStatus("Installing mods - backing up minecraft.jar...");
	if (!mcBackup.exists() && !QFile::copy(mcJar.absoluteFilePath(), mcBackup.absoluteFilePath()) )
	{
		emitFailed("Failed to back up minecraft.jar");
		return;
	}
	
	if (mcJar.isFile() && !QFile::remove(mcJar.absoluteFilePath()))
	{
		emitFailed("Failed to delete old minecraft.jar");
		return;
	}
	
	setStatus("Installing mods - Opening minecraft.jar");

	wxFFileOutputStream jarStream(mcJar.absoluteFilePath());
	wxZipOutputStream zipOut(jarStream);

	// Files already added to the jar.
	// These files will be skipped.
	QSet<QString> addedFiles;

	// Modify the jar
	setStatus("Installing mods - Adding mod files...");
	for (ModList::const_reverse_iterator iter = modList->rbegin(); iter != modList->rend(); iter++)
	{
		wxFileName modFileName = iter->GetFileName();
		setStatus("Installing mods - Adding " + modFileName.GetFullName());
		if (iter->GetModType() == Mod::ModType::MOD_ZIPFILE)
		{
			wxFFileInputStream modStream(modFileName.GetFullPath());
			wxZipInputStream zipStream(modStream);
			std::unique_ptr<wxZipEntry> entry;
			while (entry.reset(zipStream.GetNextEntry()), entry.get() != NULL)
			{
				if (entry->IsDir())
					continue;

				wxString name = entry->GetName();
				if (addedFiles.count(name) == 0)
				{
					if (!zipOut.CopyEntry(entry.release(), zipStream))
						break;
					addedFiles.insert(name);
				}
			}
		}
		else
		{
			wxFileName destFileName = modFileName;
			destFileName.MakeRelativeTo(m_inst->GetInstModsDir().GetFullPath());
			wxString destFile = destFileName.GetFullPath();

			if (addedFiles.count(destFile) == 0)
			{
				wxFFileInputStream input(modFileName.GetFullPath());
				zipOut.PutNextEntry(destFile);
				zipOut.Write(input);

				addedFiles.insert(destFile);
			}
		}
	}

	{
		wxFFileInputStream inStream(mcBackup.GetFullPath());
		wxZipInputStream zipIn(inStream);

		std::auto_ptr<wxZipEntry> entry;
		while (entry.reset(zipIn.GetNextEntry()), entry.get() != NULL)
		{
			wxString name = entry->GetName();

			if (!name.Matches("META-INF*") &&
				addedFiles.count(name) == 0)
			{
				if (!zipOut.CopyEntry(entry.release(), zipIn))
					break;
				addedFiles.insert(name);
			}
		}
	}
	
	// Recompress the jar
	TaskStep(); // STEP 3
	SetStatus(_("Installing mods - Recompressing jar..."));

	inst->SetNeedsRebuild(false);
	inst->UpdateVersion(true);
	return (ExitCode)1;
	*/
}