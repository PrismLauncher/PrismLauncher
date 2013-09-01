#include <QString>
#include <QDebug>
#include <QtXml/QtXml>
#include "OneSixAssets.h"
#include "net/DownloadJob.h"

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
				// qDebug() << trimmedf << " gets to live";
			}
			else
			{
				// DO NOT TOLERATE JUNK
				// qDebug() << trimmedf << " dies";
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

	auto firstJob = index_job->first();
	QByteArray ba = firstJob->m_data;

	QString xmlErrorMsg;
	QDomDocument doc;
	if ( !doc.setContent ( ba, false, &xmlErrorMsg ) )
	{
		qDebug() << "Failed to process s3.amazonaws.com/Minecraft.Resources. XML error:" << xmlErrorMsg << ba;
	}
	//QRegExp etag_match(".*([a-f0-9]{32}).*");
	QDomNodeList contents = doc.elementsByTagName ( "Contents" );

	DownloadJob *job = new DownloadJob();
	connect ( job, SIGNAL(succeeded()), SLOT(downloadFinished()) );
	connect ( job, SIGNAL(failed()), SIGNAL(failed()) );

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

		QString filename = fprefix + keyStr;
		QFile check_file ( filename );
		QString client_etag = "nonsense";
		// if there already is a file and md5 checking is in effect and it can be opened
		if ( check_file.exists() && check_file.open ( QIODevice::ReadOnly ) )
		{
			// check the md5 against the expected one
			client_etag = QCryptographicHash::hash ( check_file.readAll(), QCryptographicHash::Md5 ).toHex().constData();
			check_file.close();
		}
		
		QString trimmedEtag = etagStr.remove ( '"' );
		nuke_whitelist.append ( keyStr );
		if(trimmedEtag != client_etag)
		{
			job->add ( QUrl ( prefix + keyStr ), filename );
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
	DownloadJob * job = new DownloadJob(QUrl ( "http://s3.amazonaws.com/Minecraft.Resources/" ));
	connect ( job, SIGNAL(succeeded()), SLOT ( fetchXMLFinished() ) );
	index_job.reset ( job );
	job->start();
}


#include "OneSixAssets.moc"
