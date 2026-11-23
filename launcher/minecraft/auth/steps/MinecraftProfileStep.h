#pragma once
#include <QObject>

#include "minecraft/auth/AuthStep.h"
#include "net/Request.h"
#include "net/NetJob.h"

class MinecraftProfileStep : public AuthStep {
    Q_OBJECT

   public:
    MinecraftProfileStep(AccountData* data, NetworkContext* network);
    virtual ~MinecraftProfileStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private slots:
    void onRequestDone(QByteArray* response);

   private:
    Net::Request::Ptr m_request;
    NetJob::Ptr m_task;
};
