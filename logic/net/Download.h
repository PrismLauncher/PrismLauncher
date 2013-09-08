#pragma once

#include <QObject>
#include <QUrl>
#include <QSharedPointer>
#include <QNetworkReply>


enum JobStatus
{
	Job_NotStarted,
	Job_InProgress,
	Job_Finished,
	Job_Failed
};

class Download : public QObject
{
	Q_OBJECT
protected:
	explicit Download(): QObject(0){};
public:
	virtual ~Download() {};

public:
	/// the network reply
	QSharedPointer<QNetworkReply> m_reply;
	
	/// source URL
	QUrl m_url;
	
	/// The file's status
	JobStatus m_status;
	
	/// index within the parent job
	int index_within_job = 0;

signals:
	void started(int index);
	void progress(int index, qint64 current, qint64 total);
	void succeeded(int index);
	void failed(int index);

protected slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) = 0;
	virtual void downloadError(QNetworkReply::NetworkError error) = 0;
	virtual void downloadFinished() = 0;
	virtual void downloadReadyRead() = 0;
	
public slots:
	virtual void start() = 0;
};

typedef QSharedPointer<Download> DownloadPtr;
