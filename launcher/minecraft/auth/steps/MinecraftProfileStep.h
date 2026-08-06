#pragma once
#include <QObject>
#include <QUrl>

#include "minecraft/auth/AuthStep.h"
#include "net/Download.h"
#include "net/NetJob.h"

class MinecraftProfileStep : public AuthStep {
    Q_OBJECT

   public:
    explicit MinecraftProfileStep(AccountData* data);
    virtual ~MinecraftProfileStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private slots:
    void onRequestDone(QByteArray* response);

   protected:
    QUrl m_profileUrl = QUrl("https://api.minecraftservices.com/minecraft/profile");

   private:
    Net::Download::Ptr m_request;
    NetJob::Ptr m_task;
};
