#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"


class EntitlementsStep : public AuthStep {
    Q_OBJECT

public:
    explicit EntitlementsStep(AccountData *data);
    virtual ~EntitlementsStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

private slots:
    void onRequestDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

private:
    QString m_entitlementsRequestId;
};
