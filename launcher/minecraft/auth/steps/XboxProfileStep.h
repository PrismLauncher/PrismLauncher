#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"


class XboxProfileStep : public AuthStep {
    Q_OBJECT

public:
    explicit XboxProfileStep(AccountData *data);
    virtual ~XboxProfileStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

private slots:
    void onRequestDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);
};
