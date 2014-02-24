#pragma once
#include "NetAction.h"

class ScreenShot;
typedef std::shared_ptr<class ImgurUpload> ImgurUploadPtr;
class ImgurUpload : public NetAction
{
public:
	explicit ImgurUpload(ScreenShot *shot);
	static ImgurUploadPtr make(ScreenShot *shot)
	{
		return ImgurUploadPtr(new ImgurUpload(shot));
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
	ScreenShot *m_shot;
};
