#pragma once
#include "QObjectPtr.h"
#include "net/NetAction.h"
#include "Screenshot.h"

class ImgurUpload : public NetAction {
public:
    using Ptr = shared_qobject_ptr<ImgurUpload>;

    explicit ImgurUpload(ScreenShot::Ptr shot);
    static Ptr make(ScreenShot::Ptr shot) {
        return Ptr(new ImgurUpload(shot));
    }

protected
slots:
    virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    virtual void downloadError(QNetworkReply::NetworkError error);
    virtual void downloadFinished();
    virtual void downloadReadyRead() {}

public
slots:
    void startImpl() override;

private:
    ScreenShot::Ptr m_shot;
    bool finished = true;
};
