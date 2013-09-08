#pragma once
#include <QtNetwork>
#include "Download.h"
#include "ByteArrayDownload.h"
#include "FileDownload.h"
#include "CacheDownload.h"
#include "HttpMetaCache.h"

class DownloadJob;
typedef QSharedPointer<DownloadJob> DownloadJobPtr;

/**
 * A single file for the downloader/cache to process.
 */
class DownloadJob : public QObject
{
	Q_OBJECT
public:
	explicit DownloadJob(QString job_name)
		:QObject(), m_job_name(job_name){};
	
	ByteArrayDownloadPtr add(QUrl url);
	FileDownloadPtr      add(QUrl url, QString rel_target_path);
	CacheDownloadPtr     add(QUrl url, MetaEntryPtr entry);
	
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
	QString m_job_name;
	QList<DownloadPtr> downloads;
	int num_succeeded = 0;
	int num_failed = 0;
};

