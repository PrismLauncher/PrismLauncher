#include "LegacyUpdate.h"
#include "lists/LwjglVersionList.h"
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
	auto &list = LWJGLVersionList::get();
	if(!list.isLoaded())
	{
		error("Too soon! Let the LWJGL list load :)");
		emitEnded();
		return;
	}
	QString lwjglVer =  inst->lwjglVersion();
	auto version = list.getVersion(lwjglVer);
	if(!version)
	{
		error("Game update failed: the selected LWJGL version is invalid.");
		emitEnded();
	}
	lwjglVersion = version->name();
	QString url = version->url();
	
	auto &worker = NetWorker::spawn();
	QNetworkRequest req(url);
	req.setRawHeader("Host", "sourceforge.net");
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
		error("Failed to download: " + reply->errorString() + "\nSometimes you have to wait a bit if you download many LWJGL versions in a row. YMMV");
		emitEnded();
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
		QNetworkRequest req(redirectedTo);
		req.setRawHeader("Host", "sourceforge.net");
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
}
void LegacyUpdate::extractLwjgl()
{
	// make sure the directories are there
	QString lwjgl_base = PathCombine("lwjgl", lwjglVersion );
	QString nativesPath = PathCombine( lwjgl_base, "natives/");
	bool success = ensurePathExists(nativesPath);
	
	if(!success)
	{
		error("Failed to extract the lwjgl libs - error when creating required folders.");
		emitEnded();
		return;
	}
	
	QuaZip zip("lwjgl.zip");
	if(!zip.open(QuaZip::mdUnzip))
	{
		error("Failed to extract the lwjgl libs - not a valid archive.");
		emitEnded();
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
			error("Failed to extract the lwjgl libs - error while reading archive.");
			emitEnded();
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
				destFileName = PathCombine(lwjgl_base, jarNames[i]);
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
				destFileName = PathCombine(nativesPath, name);
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
	emit gameUpdateComplete();
	emitEnded();
}

void LegacyUpdate::lwjglFailed()
{
	error("Bad stuff happened while trying to get the lwjgl libs...");
	emitEnded();
}

