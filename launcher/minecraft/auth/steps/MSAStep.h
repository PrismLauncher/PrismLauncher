
#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"

#include <katabasis/DeviceFlow.h>

class MSAStep : public AuthStep {
    Q_OBJECT
public:
    enum Action {
        Refresh,
        Login
    };
public:
    explicit MSAStep(AccountData *data, Action action);
    virtual ~MSAStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

private slots:
    void onOAuthActivityChanged(Katabasis::Activity activity);

private:
    Katabasis::DeviceFlow *m_oauth2 = nullptr;
    Action m_action;
    QString m_clientId;
};
