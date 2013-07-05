#include <QDebug>
#include <QtXml/QtXml>
#include "dlqueue.h"

inline QDomElement getDomElementByTagName(QDomElement parent, QString tagname)
{
	QDomNodeList elementList = parent.elementsByTagName(tagname);
	if (elementList.count())
		return elementList.at(0).toElement();
	else
		return QDomElement();
}

class DlMachine : public QObject
{
	Q_OBJECT
public slots:
	void filesFinished()
	{
		qApp->quit();
	}

	void fetchFinished()
	{
		JobPtr firstJob = index_job->getFirstJob();
		auto DlJob = firstJob.dynamicCast<DownloadJob>();
		QByteArray ba = DlJob->m_data;
		
		QString xmlErrorMsg;
		QDomDocument doc;
		if (!doc.setContent(ba, false, &xmlErrorMsg))
		{
			qDebug() << "Failed to process s3.amazonaws.com/Minecraft.Resources. XML error:" <<
			xmlErrorMsg << ba;
		}
		QRegExp etag_match(".*([a-f0-9]{32}).*");
		QDomNodeList contents = doc.elementsByTagName("Contents");
		
		JobList *job = new JobList();
		connect(job, SIGNAL(finished()), SLOT(filesFinished()));
		
		for (int i = 0; i < contents.length(); i++)
		{
			QDomElement element = contents.at(i).toElement();
			
			if (element.isNull())
				continue;
			
			QDomElement keyElement = getDomElementByTagName(element, "Key");
			QDomElement lastmodElement = getDomElementByTagName(element, "LastModified");
			QDomElement etagElement = getDomElementByTagName(element, "ETag");
			QDomElement sizeElement = getDomElementByTagName(element, "Size");
			
			if (keyElement.isNull() || lastmodElement.isNull() || etagElement.isNull() || sizeElement.isNull())
				continue;
			
			QString keyStr = keyElement.text();
			QString lastModStr = lastmodElement.text();
			QString etagStr = etagElement.text();
			QString sizeStr = sizeElement.text();
			
			//Filter folder keys
			if (sizeStr == "0")
				continue;
			
			QString trimmedEtag = etagStr.remove('"');
			QString prefix("http://s3.amazonaws.com/Minecraft.Resources/");
			QString fprefix("assets/");
			job->add(DownloadJob::create(QUrl(prefix + keyStr),fprefix + keyStr, trimmedEtag));
		}
		files_job.reset(job);
		dl.enqueue(files_job);
	}
	void fetchStarted()
	{
		qDebug() << "Started downloading!";
	}
public:
	void start()
	{
		JobList *job = new JobList();
		job->add(DownloadJob::create(QUrl("http://s3.amazonaws.com/Minecraft.Resources/")));
		connect(job, SIGNAL(finished()), SLOT(fetchFinished()));
		connect(job, SIGNAL(started()), SLOT(fetchStarted()));
		index_job.reset(job);
		dl.enqueue(index_job);
	}
	JobListQueue dl;
	JobListPtr index_job;
	JobListPtr files_job;
};

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	
	DlMachine dl;
	dl.start();
	
	return app.exec();
}

#include "asset_test.moc"
