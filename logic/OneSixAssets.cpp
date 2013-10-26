#include <QString>
#include <logger/QsLog.h>
#include <QtXml/QtXml>
#include "OneSixAssets.h"
#include "net/DownloadJob.h"
#include "net/HttpMetaCache.h"
#include "MultiMC.h"

inline QDomElement getDomElementByTagName(QDomElement parent, QString tagname)
{
	QDomNodeList elementList = parent.elementsByTagName(tagname);
	if (elementList.count())
		return elementList.at(0).toElement();
	else
		return QDomElement();
}

class ThreadedDeleter : public QThread
{
	Q_OBJECT
public:
	void run()
	{
		QLOG_INFO() << "Cleaning up assets folder...";
		QDirIterator iter ( m_base, QDirIterator::Subdirectories );
		int base_length = m_base.length();
		while ( iter.hasNext() )
		{
			QString filename = iter.next();
			QFileInfo current ( filename );
			// we keep the dirs... whatever
			if ( current.isDir() )
				continue;
			QString trimmedf = filename;
			trimmedf.remove ( 0, base_length + 1 );
			if ( m_whitelist.contains ( trimmedf ) )
			{
				QLOG_TRACE() << trimmedf << " gets to live";
			}
			else
			{
				// DO NOT TOLERATE JUNK
				QLOG_TRACE() << trimmedf << " dies";
				QFile f ( filename );
				f.remove();
			}
		}
	}
	QString m_base;
	QStringList m_whitelist;
};

void OneSixAssets::downloadFinished()
{
	deleter = new ThreadedDeleter();
	QDir dir("assets");
	deleter->m_base = dir.absolutePath();
	deleter->m_whitelist = nuke_whitelist;
	connect(deleter, SIGNAL(finished()), SIGNAL(finished()));
	deleter->start();
}


void OneSixAssets::fetchXMLFinished()
{
	QString prefix ( "http://s3.amazonaws.com/Minecraft.Resources/" );
	QString fprefix ( "assets/" );
	nuke_whitelist.clear();

	emit filesStarted();

	auto firstJob = index_job->first();
	QByteArray ba  = std::dynamic_pointer_cast<ByteArrayDownload>(firstJob)->m_data;

	QString xmlErrorMsg;
	QDomDocument doc;
	if ( !doc.setContent ( ba, false, &xmlErrorMsg ) )
	{
		QLOG_ERROR() << "Failed to process s3.amazonaws.com/Minecraft.Resources. XML error:" << xmlErrorMsg << ba;
		emit failed();
		return;
	}
	//QRegExp etag_match(".*([a-f0-9]{32}).*");
	QDomNodeList contents = doc.elementsByTagName ( "Contents" );

	DownloadJob *job = new DownloadJob("Assets");
	connect ( job, SIGNAL(succeeded()), SLOT(downloadFinished()) );
	connect ( job, SIGNAL(failed()), SIGNAL(failed()) );
	connect ( job, SIGNAL(filesProgress(int, int, int)), SIGNAL(filesProgress(int, int, int)) );

	auto metacache = MMC->metacache();
	
	for ( int i = 0; i < contents.length(); i++ )
	{
		QDomElement element = contents.at ( i ).toElement();

		if ( element.isNull() )
			continue;

		QDomElement keyElement = getDomElementByTagName ( element, "Key" );
		QDomElement lastmodElement = getDomElementByTagName ( element, "LastModified" );
		QDomElement etagElement = getDomElementByTagName ( element, "ETag" );
		QDomElement sizeElement = getDomElementByTagName ( element, "Size" );

		if ( keyElement.isNull() || lastmodElement.isNull() || etagElement.isNull() || sizeElement.isNull() )
			continue;

		QString keyStr = keyElement.text();
		QString lastModStr = lastmodElement.text();
		QString etagStr = etagElement.text();
		QString sizeStr = sizeElement.text();

		//Filter folder keys
		if ( sizeStr == "0" )
			continue;

		nuke_whitelist.append ( keyStr );
		
		auto entry = metacache->resolveEntry("assets", keyStr, etagStr);
		if(entry->stale)
		{
			job->addCacheDownload(QUrl(prefix + keyStr), entry);
		}
	}
	if(job->size())
	{
		files_job.reset ( job );
		files_job->start();
	}
	else
	{
		delete job;
		emit finished();
	}
}

void OneSixAssets::start()
{
	auto job = new DownloadJob("Assets index");
	job->addByteArrayDownload(QUrl ( "http://s3.amazonaws.com/Minecraft.Resources/" ));
	connect ( job, SIGNAL(succeeded()), SLOT ( fetchXMLFinished() ) );
	emit indexStarted();
	index_job.reset ( job );
	job->start();
}

#include "OneSixAssets.moc"
