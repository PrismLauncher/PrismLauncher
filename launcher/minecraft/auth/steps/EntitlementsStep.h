#pragma once
#include <QObject>

#include "minecraft/auth/AuthStep.h"
#include "net/Request.h"
#include "net/NetJob.h"

class EntitlementsStep : public AuthStep {
    Q_OBJECT

   public:
    EntitlementsStep(AccountData* data, NetworkContext* network);
    virtual ~EntitlementsStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private slots:
    void onRequestDone(QByteArray* response);

   private:
    QString m_entitlements_request_id;
    Net::Request::Ptr m_request;
    NetJob::Ptr m_task;
};
