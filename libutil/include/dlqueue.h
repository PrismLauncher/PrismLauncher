#pragma once
#include "jobqueue.h"
#include <QtNetwork>

/**
 * A single file for the downloader/cache to process.
 */
class LIBUTIL_EXPORT DownloadJob : public Job
{
	Q_OBJECT
public:
	DownloadJob(QUrl url, QString rel_target_path = QString(), QString expected_md5 = QString());
	static JobPtr create(QUrl url, QString rel_target_path = QString(), QString expected_md5 = QString());
	
	
public:
	static bool ensurePathExists(QString filenamepath);
public slots:
	virtual void start();
	
private slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);;
	void downloadError(QNetworkReply::NetworkError error);
	void downloadFinished();
	void downloadReadyRead();
	
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
	QString m_target_path;
	/// this is the output file, if any
	QFile m_output_file;
	/// if not saving to file, downloaded data is placed here
	QByteArray m_data;
	
	/// The file's status
	JobStatus m_status;
};
