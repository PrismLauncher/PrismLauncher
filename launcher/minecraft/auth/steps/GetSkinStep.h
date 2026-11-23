#pragma once
#include <QObject>

#include "minecraft/auth/AuthStep.h"
#include "net/Request.h"
#include "net/NetJob.h"

class GetSkinStep : public AuthStep {
    Q_OBJECT

   public:
    GetSkinStep(AccountData* data, NetworkContext* network);
    virtual ~GetSkinStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private slots:
    void onRequestDone(QByteArray* response);

   private:
    Net::Request::Ptr m_request;
    NetJob::Ptr m_task;
};
