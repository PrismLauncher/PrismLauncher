#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"

class GetSkinStep : public AuthStep {
    Q_OBJECT

   public:
    explicit GetSkinStep(AccountData* data);
    virtual ~GetSkinStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

   private slots:
    void onRequestDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);
};
