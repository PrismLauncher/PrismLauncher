#pragma once

#include "net/NetRequest.h"

class CapeChange : public Net::NetRequest {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<CapeChange>;
    CapeChange(QString token, QString capeId);
    virtual ~CapeChange() = default;

    static CapeChange::Ptr make(QString token, QString capeId);
    void init() override;

   protected:
    virtual QNetworkReply* getReply(QNetworkRequest&) override;

   private:
    QString m_capeId;
    QString m_token;
};
