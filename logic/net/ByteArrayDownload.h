#pragma once
#include "NetAction.h"

typedef std::shared_ptr<class ByteArrayDownload> ByteArrayDownloadPtr;
class ByteArrayDownload : public NetAction
{
	Q_OBJECT
public:
	ByteArrayDownload(QUrl url);
	static ByteArrayDownloadPtr make(QUrl url)
	{
		return ByteArrayDownloadPtr(new ByteArrayDownload(url));
	}

public:
	/// if not saving to file, downloaded data is placed here
	QByteArray m_data;

public
slots:
	virtual void start();

protected
slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadError(QNetworkReply::NetworkError error);
	void downloadFinished();
	void downloadReadyRead();
};
