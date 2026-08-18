#pragma once
#include <QObject>

#include "minecraft/auth/AuthStep.h"
#include "net/NetJob.h"
#include "net/NetRequest.h"

class LauncherLoginStep : public AuthStep {
    Q_OBJECT

   public:
    explicit LauncherLoginStep(AccountData* data);
    virtual ~LauncherLoginStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private slots:
    void onRequestDone(QByteArray* response);

   private:
    Net::NetRequest::Ptr m_request;
    NetJob::Ptr m_task;
};
