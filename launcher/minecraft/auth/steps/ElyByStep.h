#pragma once

#include "minecraft/auth/AuthStep.h"

#include "QObjectPtr.h"

namespace Net {
class Upload;
}

class NetJob;

class ElyByStep : public AuthStep {
    Q_OBJECT

   public:
    explicit ElyByStep(AccountData* data, bool refresh);
    ~ElyByStep() noexcept override = default;

    QString describe() override;

   public slots:
    void perform() override;

   private:
    void onRequestDone(QByteArray* response);

   private:
    bool m_refresh = false;
    shared_qobject_ptr<NetJob> m_task;
    shared_qobject_ptr<Net::Upload> m_request;
};
