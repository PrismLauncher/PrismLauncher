#pragma once

#include "net/NetRequest.h"

class SkinUpload : public Net::NetRequest {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<SkinUpload>;
    enum Model { STEVE, ALEX };

    // Note this class takes ownership of the file.
    SkinUpload(QString token, QByteArray skin, Model model = STEVE);
    virtual ~SkinUpload() = default;

    static SkinUpload::Ptr make(QString token, QByteArray skin, Model model = STEVE);
    void init() override;

   protected:
    virtual QNetworkReply* getReply(QNetworkRequest&) override;

   private:
    Model m_model;
    QByteArray m_skin;
    QString m_token;
};
