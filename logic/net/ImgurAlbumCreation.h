#pragma once
#include "NetAction.h"

class ScreenShot;
typedef std::shared_ptr<class ImgurAlbumCreation> ImgurAlbumCreationPtr;
class ImgurAlbumCreation : public NetAction
{
public:
	explicit ImgurAlbumCreation(QList<ScreenShot *> screenshots);
	static ImgurAlbumCreationPtr make(QList<ScreenShot *> screenshots)
	{
		return ImgurAlbumCreationPtr(new ImgurAlbumCreation(screenshots));
	}

	QString deleteHash() const
	{
		return m_deleteHash;
	}
	QString id() const
	{
		return m_id;
	}

protected
slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	virtual void downloadError(QNetworkReply::NetworkError error);
	virtual void downloadFinished();
	virtual void downloadReadyRead()
	{
	}

public
slots:
	virtual void start();

private:
	QList<ScreenShot *> m_screenshots;

	QString m_deleteHash;
	QString m_id;
};
