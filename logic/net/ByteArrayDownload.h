#pragma once
#include "Download.h"

class ByteArrayDownload: public Download
{
	Q_OBJECT
public:
	ByteArrayDownload(QUrl url);
	
public:
	/// if not saving to file, downloaded data is placed here
	QByteArray m_data;
	
public slots:
	virtual void start();
	
protected slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadError(QNetworkReply::NetworkError error);
	void downloadFinished();
	void downloadReadyRead();
};

typedef std::shared_ptr<ByteArrayDownload> ByteArrayDownloadPtr;
