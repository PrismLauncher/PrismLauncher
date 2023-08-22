#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"

class MinecraftProfileStep : public AuthStep {
    Q_OBJECT

   public:
    explicit MinecraftProfileStep(AccountData* data);
    virtual ~MinecraftProfileStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

   private slots:
    void onRequestDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);
};
