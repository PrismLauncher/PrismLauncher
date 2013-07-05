#include <QtCore>
#include <QtNetwork>
#include <QDebug>
#include <QtXml/QtXml>

enum JobStatus
{
	Job_NotStarted,
	Job_InProgress,
	Job_Finished,
	Job_Failed
};

class JobList;

class Job : public QObject
{
	Q_OBJECT
protected:
	explicit Job(): QObject(0){};
public:
	virtual ~Job() {};
signals:
	void finish();
	void fail();
	void progress(qint64 current, qint64 total);
public slots:
	virtual void start() = 0;
};
typedef QSharedPointer<Job> JobPtr;

/**
 * A list of jobs, to be processed one by one.
 */
class JobList : public QObject
{
	friend class JobListQueue;
	Q_OBJECT
public:

	JobList() : QObject(0)
	{
		m_status = Job_NotStarted;
		current_job_idx = 0;
	}
	JobStatus getStatus()
	{
		return m_status;
	}
	void add(JobPtr dlable)
	{
		if(m_status == Job_NotStarted)
			m_jobs.append(dlable);
		//else there's a bug. TODO: catch the bugs
	}
	JobPtr getFirstJob()
	{
		if(m_jobs.size())
			return m_jobs[0];
		else
			return JobPtr();
	}
	void start()
	{
		current_job_idx = 0;
		auto job = m_jobs[current_job_idx];
		
		connect(job.data(), SIGNAL(progress(qint64,qint64)), SLOT(currentJobProgress(qint64,qint64)));
		connect(job.data(), SIGNAL(finish()), SLOT(currentJobFinished()));
		connect(job.data(), SIGNAL(fail()), SLOT(currentJobFailed()));
		job->start();
		emit started();
	}
private slots:
	void currentJobFinished()
	{
		if(current_job_idx == m_jobs.size() - 1)
		{
			m_status = Job_Finished;
			emit finished();
		}
		else
		{
			current_job_idx++;
			auto job = m_jobs[current_job_idx];
			connect(job.data(), SIGNAL(progress(qint64,qint64)), SLOT(currentJobProgress(qint64,qint64)));
			connect(job.data(), SIGNAL(finish()), SLOT(currentJobFinished()));
			connect(job.data(), SIGNAL(fail()), SLOT(currentJobFailed()));
			job->start();
		}
	}
	void currentJobFailed()
	{
		m_status = Job_Failed;
		emit failed();
	}
	void currentJobProgress(qint64 current, qint64 total)
	{
		if(!total)
			return;
		
		int total_jobs = m_jobs.size();
		
		if(!total_jobs)
			return;
		
		float job_chunk = 1000.0 / float(total_jobs);
		float cur = current;
		float tot = total;
		float last_chunk = (cur / tot) * job_chunk;
		
		float list_total = job_chunk * current_job_idx + last_chunk;
		emit progress(qint64(list_total), 1000LL);
	}
private:
	QVector<JobPtr> m_jobs;
	/// The overall status of this job list
	JobStatus m_status;
	int current_job_idx;
signals:
	void progress(qint64 current, qint64 total);
	void started();
	void finished();
	void failed();
};
typedef QSharedPointer<JobList> JobListPtr;


/**
 * A queue of job lists! The job lists fail or finish as units.
 */
class JobListQueue : public QObject
{
	Q_OBJECT
public:
	JobListQueue(QObject *p = 0):
		QObject(p),
		nam(new QNetworkAccessManager()),
		currentIndex(0),
		is_running(false){}
	
	void enqueue(JobListPtr job)
	{
		jobs.enqueue(job);
		
		// finish or fail, we should catch that and start the next one
		connect(job.data(),SIGNAL(finished()), SLOT(startNextJob()));
		connect(job.data(),SIGNAL(failed()), SLOT(startNextJob()));
		
		if(!is_running)
		{
			QTimer::singleShot(0, this, SLOT(startNextJob()));
		}
	}
	
private slots:
	void startNextJob()
	{
		if (jobs.isEmpty())
		{
			currentJobList.clear();
			currentIndex = 0;
			is_running = false;
			emit finishedAllJobs();
			return;
		}
		
		currentJobList = jobs.dequeue();
		is_running = true;
		currentIndex = 0;
		currentJobList->start();
	}
	
signals:
	void finishedAllJobs();
	
private:
	JobListPtr currentJobList;
	QQueue<JobListPtr> jobs;
	QSharedPointer<QNetworkAccessManager> nam;
	unsigned currentIndex;
	bool is_running;
};

/**
 * A single file for the downloader/cache to process.
 */
class DownloadJob : public Job
{
	friend class Downloader;
	Q_OBJECT
private:
	DownloadJob(QUrl url, QString rel_target_path = QString(), QString expected_md5 = QString())
		:Job()
	{
		m_url = url;
		m_rel_target_path = rel_target_path;
		m_expected_md5 = expected_md5;
		
		m_check_md5 = m_expected_md5.size();
		m_save_to_file = m_rel_target_path.size();
		m_status = Job_NotStarted;
	};
public:
	static JobPtr create(QUrl url, QString rel_target_path = QString(), QString expected_md5 = QString())
	{
		return JobPtr(new DownloadJob(url, rel_target_path, expected_md5));
	}
public slots:
	virtual void start()
	{
		m_manager.reset(new QNetworkAccessManager());
		if(m_save_to_file)
		{
			QString filename = m_rel_target_path;
			m_output_file.setFileName(filename);
			// if there already is a file and md5 checking is in effect
			if(m_output_file.exists() && m_check_md5)
			{
				// and it can be opened
				if(m_output_file.open(QIODevice::ReadOnly))
				{
					// check the md5 against the expected one
					QString hash = QCryptographicHash::hash(m_output_file.readAll(), QCryptographicHash::Md5).toHex().constData();
					m_output_file.close();
					// skip this file if they match
					if(hash == m_expected_md5)
					{
						qDebug() << "Skipping " << m_url.toString() << ": md5 match.";
						emit finish();
						return;
					}
				}
			}
			QFileInfo a(filename);
			QDir dir;
			if(!dir.mkpath(a.path()))
			{
				/*
				 * error when making the folder structure
				 */
				emit fail();
				return;
			}
			if (!m_output_file.open(QIODevice::WriteOnly))
			{
				/*
				 * Can't open the file... the job failed
				 */
				emit fail();
				return;
			}
		}
		qDebug() << "Downloading " << m_url.toString();
		QNetworkRequest request(m_url);
		QNetworkReply * rep = m_manager->get(request);
		m_reply = QSharedPointer<QNetworkReply>(rep, &QObject::deleteLater);
		connect(rep, SIGNAL(downloadProgress(qint64,qint64)), SLOT(downloadProgress(qint64,qint64)));
		connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
		connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
		connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
	};
private slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
	{
		emit progress(bytesReceived, bytesTotal);
	};
	
	void downloadError(QNetworkReply::NetworkError error)
	{
		// error happened during download.
		// TODO: log the reason why
		m_status = Job_Failed;
	}
	
	void downloadFinished()
	{
		// if the download succeeded
		if(m_status != Job_Failed)
		{
			// nothing went wrong...
			m_status = Job_Finished;
			// save the data to the downloadable if we aren't saving to file
			if(!m_save_to_file)
			{
				m_data = m_reply->readAll();
			}
			else
			{
				m_output_file.close();
			}
			
			//TODO: check md5 here!
			m_reply.clear();
			emit finish();
			return;
		}
		// else the download failed
		else
		{
			if(m_save_to_file)
			{
				m_output_file.close();
				m_output_file.remove();
			}
			m_reply.clear();
			emit fail();
			return;
		}
	}
	void downloadReadyRead()
	{
		if(m_save_to_file)
		{
			m_output_file.write( m_reply->readAll());
		}
	};
	
public:
	/// the associated network manager
	QSharedPointer<QNetworkAccessManager> m_manager;
	/// the network reply
	QSharedPointer<QNetworkReply> m_reply;
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
	/// this is the output file, if any
	QFile m_output_file;
	/// if not saving to file, downloaded data is placed here
	QByteArray m_data;
	
	
	/// The file's status
	JobStatus m_status;
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
