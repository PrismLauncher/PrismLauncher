#pragma once
#include <QObject>

#include "minecraft/auth/AuthStep.h"
#include "net/Download.h"
#include "net/NetJob.h"

class EntitlementsStep : public AuthStep {
    Q_OBJECT

   public:
    explicit EntitlementsStep(AccountData* data);
    virtual ~EntitlementsStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private slots:
    void onRequestDone(QByteArray* response);

   private:
    QString m_entitlements_request_id;
    Net::Download::Ptr m_request;
    NetJob::Ptr m_task;
};
