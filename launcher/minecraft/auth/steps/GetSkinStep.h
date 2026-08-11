#pragma once
#include <QObject>

#include "minecraft/auth/AuthStep.h"
#include "net/NetRequest.h"
#include "net/NetJob.h"

class GetSkinStep : public AuthStep {
    Q_OBJECT

   public:
    explicit GetSkinStep(AccountData* data);
    virtual ~GetSkinStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private slots:
    void onRequestDone(QByteArray* response);

   private:
    Net::NetRequest::Ptr m_request;
    NetJob::Ptr m_task;
};
