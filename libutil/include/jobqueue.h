#pragma once
#include <QtCore>

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
	unsigned currentIndex;
	bool is_running;
};
