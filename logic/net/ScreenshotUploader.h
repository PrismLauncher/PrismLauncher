#pragma once
#include "NetAction.h"

class ScreenShot;
typedef std::shared_ptr<class ScreenShotUpload> ScreenShotUploadPtr;
typedef std::shared_ptr<class ScreenShotGet> ScreenShotGetPtr;
class ScreenShotUpload : public NetAction
{
public:
	explicit ScreenShotUpload(ScreenShot *shot);
	static ScreenShotUploadPtr make(ScreenShot *shot)
	{
		return ScreenShotUploadPtr(new ScreenShotUpload(shot));
	}

protected
slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	virtual void downloadError(QNetworkReply::NetworkError error);
	virtual void downloadFinished();
	virtual void downloadReadyRead();

public
slots:
	virtual void start();

private:
	ScreenShot *m_shot;
};
