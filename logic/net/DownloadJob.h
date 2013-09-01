#pragma once
#include <QtNetwork>

class DownloadJob;
class Download;
typedef QSharedPointer<DownloadJob> DownloadJobPtr;
typedef QSharedPointer<Download> DownloadPtr;

enum JobStatus
{
	Job_NotStarted,
	Job_InProgress,
	Job_Finished,
	Job_Failed
};

class Job : public QObject
{
	Q_OBJECT
protected:
	explicit Job(): QObject(0){};
public:
	virtual ~Job() {};

public slots:
	virtual void start() = 0;
};

class Download: public Job
{
	friend class DownloadJob;
	Q_OBJECT
protected:
	Download(QUrl url, QString rel_target_path = QString(), QString expected_md5 = QString());
public:
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
	/// is the saving file already open?
	bool m_opened_for_saving;
	/// if saving to file, use the one specified in this string
	QString m_target_path;
	/// this is the output file, if any
	QFile m_output_file;
	/// if not saving to file, downloaded data is placed here
	QByteArray m_data;
	
	int currentProgress = 0;
	int totalProgress = 0;
	
	/// The file's status
	JobStatus m_status;
	
	int index_within_job = 0;
signals:
	void started(int index);
	void progress(int index, qint64 current, qint64 total);
	void succeeded(int index);
	void failed(int index);
public slots:
	virtual void start();
	
private slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);;
	void downloadError(QNetworkReply::NetworkError error);
	void downloadFinished();
	void downloadReadyRead();
};

/**
 * A single file for the downloader/cache to process.
 */
class DownloadJob : public Job
{
	Q_OBJECT
public:
	explicit DownloadJob()
		:Job(){};
	explicit DownloadJob(QUrl url, QString rel_target_path = QString(), QString expected_md5 = QString())
		:Job()
	{
		add(url, rel_target_path, expected_md5);
	};
	
	DownloadPtr add(QUrl url, QString rel_target_path = QString(), QString expected_md5 = QString());
	DownloadPtr operator[](int index)
	{
		return downloads[index];
	};
	DownloadPtr first()
	{
		if(downloads.size())
			return downloads[0];
		return DownloadPtr();
	}
	int size() const
	{
		return downloads.size();
	}
signals:
	void started();
	void progress(qint64 current, qint64 total);
	void succeeded();
	void failed();
public slots:
	virtual void start();
private slots:
	void partProgress(int index, qint64 bytesReceived, qint64 bytesTotal);;
	void partSucceeded(int index);
	void partFailed(int index);
private:
	QList<DownloadPtr> downloads;
	int num_succeeded = 0;
	int num_failed = 0;
};

