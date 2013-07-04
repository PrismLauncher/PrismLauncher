#include <QtCore>
#include <QtNetwork>
#include <QDebug>
#include <QtXml/QtXml>

enum DlStatus
{
	Dl_NotStarted,
	Dl_InProgress,
	Dl_Finished,
	Dl_Failed
};

/**
 * A single file for the downloader/cache to process.
 */
struct Downloadable
{
	Downloadable(QUrl url, QString rel_target_path = QString(), QString expected_md5 = QString())
		:m_url(url), m_rel_target_path(rel_target_path), m_expected_md5(expected_md5)
	{
		m_check_md5 = m_expected_md5.size();
		m_save_to_file = m_rel_target_path.size();
		status = Dl_NotStarted;
	};
	/// source URL
	QUrl m_url;
	
	/// if true, check the md5sum against a provided md5sum
	/// also, if a file exists, perform an md5sum first and don't download only if they don't match
	bool m_check_md5;
	/// the expected md5 checksum
	QString m_expected_md5;
	
	/// save to file?
	bool m_save_to_file;
	/// if saving to file, use the one specified in this string
	QString m_rel_target_path;
	/// if not saving to file, downloaded data is placed here
	QByteArray data;
	/// The file's status
	DlStatus status;
};
typedef QSharedPointer<Downloadable> DownloadablePtr;

class Downloader;

/**
 * A downloader job, which can have multiple files
 * the user of the downloader is responsible for creating it
 * and connecting its signals to his own slots.
 */
class DLJob : public QObject
{
	friend class Downloader;
	Q_OBJECT
public:

	DLJob() : QObject(0) {}
	DLJob (QUrl url, QString rel_target_path = QString(), QString expected_md5 = QString() ) : QObject(0)
	{
		m_status = Dl_NotStarted;
		append(url, rel_target_path, expected_md5);
	}
	DlStatus getStatus()
	{
		return m_status;
	}
	void append (QUrl url, QString target = QString(), QString md5 = QString())
	{
		Downloadable * dlable = new Downloadable(url, target, md5);
		m_downloads.append(DownloadablePtr(dlable));
	}
	QByteArray getFirstFileData()
	{
		if(!m_downloads.size())
			return QByteArray();
		else return m_downloads[0]->data;
	}
	void add(DownloadablePtr dlable)
	{
		m_downloads.append(dlable);
	}
private:
	void emitStart()
	{
		m_status = Dl_InProgress;
		emit started();
	}
	void emitFail()
	{
		m_status = Dl_Failed;
		emit failed();
	}
	void emitFinish()
	{
		m_status = Dl_Finished;
		emit finished();
	}
private:
	QVector<DownloadablePtr> m_downloads;
	/// The job's status
	DlStatus m_status;
signals:
	void started();
	void finished();
	void failed();
};
typedef QSharedPointer<DLJob> DLJobPtr;

/**
 * The downloader itself. Make one, use it.
 * User is responsible for keeping it around until all the jobs either finish or fail.
 */
class Downloader : public QObject
{
	Q_OBJECT
public:
	Downloader(QObject *p = 0):
		QObject(p),
		nam(new QNetworkAccessManager(this)),
		currentReply(0),
		currentIndex(0){}
	
	void enqueue(DLJobPtr job)
	{
		if(jobs.empty())
		{
			qDebug() << "NEXT JOB!";
			QTimer::singleShot(0, this, SLOT(startNextJob()));
		}
		jobs.enqueue(job);
	}
	
	private slots:
	void startNextJob()
	{
		if (jobs.isEmpty())
		{
			currentJob.clear();
			currentIndex = 0;
			emit finishedAllJobs();
			return;
		}
		
		currentJob = jobs.dequeue();
		currentIndex = 0;
		currentJob->emitStart();
		
		startNextDownload();
	}
	
	void startNextDownload()
	{
		// we finished the current job. Good job.
		if(currentIndex >= currentJob->m_downloads.size())
		{
			currentJob->emitFinish();
			qDebug() << "NEXT JOB!";
			QTimer::singleShot(0, this, SLOT(startNextJob()));
			return;
		}
		
		DownloadablePtr dlable = currentJob->m_downloads[currentIndex];
		if(dlable->m_save_to_file)
		{
			QString filename = dlable->m_rel_target_path;
			currentOutput.setFileName(filename);
			// if there already is a file and md5 checking is in effect
			if(currentOutput.exists() && dlable->m_check_md5)
			{
				// and it can be opened
				if(currentOutput.open(QIODevice::ReadOnly))
				{
					// check the md5 against the expected one
					QString hash = QCryptographicHash::hash(currentOutput.readAll(), QCryptographicHash::Md5).toHex().constData();
					currentOutput.close();
					// skip this file if they match
					if(hash == dlable->m_expected_md5)
					{
						currentIndex++;
						QTimer::singleShot(0, this, SLOT(startNextDownload()));
						return;
					}
				}
			}
			QFileInfo a(filename);
			QDir dir;
			if(!dir.mkpath(a.path()))
			{
				/*
				 * TODO: error when making the folder structure
				 */
				currentJob->emitFail();
				currentJob.clear();
				currentIndex = 0;
				QTimer::singleShot(0, this, SLOT(startNextJob()));
				qDebug() << "NEXT JOB!";
				return;
			}
			if (!currentOutput.open(QIODevice::WriteOnly))
			{
				/*
				 * TODO: Can't open the file... the job failed
				 */
				currentJob->emitFail();
				currentJob.clear();
				currentIndex = 0;
				QTimer::singleShot(0, this, SLOT(startNextJob()));
				qDebug() << "NEXT JOB!";
				return;
			}
		}

		QNetworkRequest request(dlable->m_url);
		QNetworkReply * rep = nam->get(request);
		currentReply = QSharedPointer<QNetworkReply>(rep, &QObject::deleteLater);
		connect(rep, SIGNAL(downloadProgress(qint64,qint64)), SLOT(downloadProgress(qint64,qint64)));
		connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
		connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
		connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
	}
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
	{
		// eventually, progress bars. yeah.
	};
	
	void downloadError(QNetworkReply::NetworkError error)
	{
		// error happened during download. :<
		DownloadablePtr dlable = currentJob->m_downloads[currentIndex];
		//TODO: log the reason why
		dlable->status = Dl_Failed;
	}
	
	void downloadFinished()
	{
		DownloadablePtr dlable = currentJob->m_downloads[currentIndex];
		// if the download succeeded
		if(dlable->status != Dl_Failed)
		{
			// nothing went wrong...
			dlable->status = Dl_Finished;
			// save the data to the downloadable if we aren't saving to file
			if(!dlable->m_save_to_file)
			{
				dlable->data = currentReply->readAll();
			}
			else
			{
				currentOutput.close();
			}
			
			//TODO: check md5 here!
			
			// continue with the next download, if any
			currentIndex ++;
			QTimer::singleShot(0, this, SLOT(startNextDownload()));
			return;
		}
		// else the download failed
		else
		{
			if(dlable->m_save_to_file)
			{
				currentOutput.close();
				currentOutput.remove();
			}
		}
	}
	void downloadReadyRead()
	{
		DownloadablePtr dlable = currentJob->m_downloads[currentIndex];
		if(dlable->m_save_to_file)
		{
			currentOutput.write(currentReply->readAll());
		}
	};

signals:
	void finishedAllJobs();
	
public slots:
	
private:
	QSharedPointer<QNetworkAccessManager> nam;
	DLJobPtr currentJob;
	QSharedPointer<QNetworkReply> currentReply;
	QQueue<DLJobPtr> jobs;
	QFile currentOutput;
	unsigned currentIndex;
};

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
		QByteArray ba = index_job->getFirstFileData();
		
		QString xmlErrorMsg;
		QDomDocument doc;
		if (!doc.setContent(ba, false, &xmlErrorMsg))
		{
			qDebug() << "Failed to process s3.amazonaws.com/Minecraft.Resources. XML error:" <<
			xmlErrorMsg << ba;
		}
		QRegExp etag_match(".*([a-f0-9]{32}).*");
		QDomNodeList contents = doc.elementsByTagName("Contents");
		
		DLJob *job = new DLJob();
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
			Downloadable * d = new Downloadable(QUrl(prefix + keyStr),fprefix + keyStr, trimmedEtag);
			
			job->add(DownloadablePtr(d));
			
			//qDebug() << keyStr << " " << lastModStr << " " << etagStr << sizeStr;
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
		DLJob *job = new DLJob(QUrl("http://s3.amazonaws.com/Minecraft.Resources/"));
		connect(job, SIGNAL(finished()), SLOT(fetchFinished()));
		connect(job, SIGNAL(started()), SLOT(fetchStarted()));
		index_job.reset(job);
		dl.enqueue(index_job);
	}
	Downloader dl;
	DLJobPtr index_job;
	DLJobPtr files_job;
};

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	
	DlMachine dl;
	dl.start();
	
	return app.exec();
}

#include "asset_test.moc"
