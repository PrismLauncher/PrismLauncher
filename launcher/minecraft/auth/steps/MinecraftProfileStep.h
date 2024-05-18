#pragma once
#include <QObject>
#include <memory>

#include "minecraft/auth/AuthStep.h"
#include "net/Download.h"

class MinecraftProfileStep : public AuthStep {
    Q_OBJECT

   public:
    explicit MinecraftProfileStep(AccountData* data);
    virtual ~MinecraftProfileStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private slots:
    void onRequestDone();

   private:
    std::shared_ptr<QByteArray> m_response;
    Net::Download::Ptr m_task;
};
