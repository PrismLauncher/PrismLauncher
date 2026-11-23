#pragma once
#include <QObject>

#include "minecraft/auth/AuthStep.h"
#include "net/NetJob.h"
#include "net/Request.h"

class XboxUserStep : public AuthStep {
    Q_OBJECT

   public:
    XboxUserStep(AccountData* data, NetworkContext* network);
    virtual ~XboxUserStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private slots:
    void onRequestDone(QByteArray* response);

   private:
    Net::Request::Ptr m_request;
    NetJob::Ptr m_task;
};
