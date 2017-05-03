#pragma once
#include "net/NetAction.h"
#include "Screenshot.h"

#include "multimc_logic_export.h"

typedef shared_qobject_ptr<class ImgurAlbumCreation> ImgurAlbumCreationPtr;
class MULTIMC_LOGIC_EXPORT ImgurAlbumCreation : public NetAction
{
public:
	explicit ImgurAlbumCreation(QList<ScreenshotPtr> screenshots);
	static ImgurAlbumCreationPtr make(QList<ScreenshotPtr> screenshots)
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
	virtual void executeTask();

private:
	QList<ScreenshotPtr> m_screenshots;

	QString m_deleteHash;
	QString m_id;
};
