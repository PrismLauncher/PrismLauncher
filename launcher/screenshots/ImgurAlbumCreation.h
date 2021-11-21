#pragma once
#include "net/NetAction.h"
#include "Screenshot.h"
#include "QObjectPtr.h"

typedef shared_qobject_ptr<class ImgurAlbumCreation> ImgurAlbumCreationPtr;
class ImgurAlbumCreation : public NetAction
{
public:
    explicit ImgurAlbumCreation(QList<ScreenShot::Ptr> screenshots);
    static ImgurAlbumCreationPtr make(QList<ScreenShot::Ptr> screenshots)
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
    virtual void startImpl();

private:
    QList<ScreenShot::Ptr> m_screenshots;

    QString m_deleteHash;
    QString m_id;
};
