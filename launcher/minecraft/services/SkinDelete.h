#pragma once

#include "net/NetRequest.h"

class SkinDelete : public Net::NetRequest {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<SkinDelete>;
    SkinDelete(QString token);
    virtual ~SkinDelete() = default;

    static SkinDelete::Ptr make(QString token);
    void init() override;

   protected:
    virtual QNetworkReply* getReply(QNetworkRequest&) override;

   private:
    QString m_token;
};
