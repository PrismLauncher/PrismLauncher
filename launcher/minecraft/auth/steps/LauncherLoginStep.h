#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"


class LauncherLoginStep : public AuthStep {
    Q_OBJECT

public:
    explicit LauncherLoginStep(AccountData *data);
    virtual ~LauncherLoginStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

private slots:
    void onRequestDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);
};
