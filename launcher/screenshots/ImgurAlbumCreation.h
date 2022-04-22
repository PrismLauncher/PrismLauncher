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
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) override;
    void downloadError(QNetworkReply::NetworkError error) override;
    void downloadFinished() override;
    void downloadReadyRead() override {}

public
slots:
    void executeTask() override;

private:
    QList<ScreenShot::Ptr> m_screenshots;

    QString m_deleteHash;
    QString m_id;
};
