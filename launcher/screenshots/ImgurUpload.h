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
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) override;
    void downloadError(QNetworkReply::NetworkError error) override;
    void downloadFinished() override;
    void downloadReadyRead() override {}

public
slots:
    void executeTask() override;

private:
    ScreenShot::Ptr m_shot;
    bool finished = true;
};
