#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"

#include <katabasis/DeviceFlow.h>

class OfflineStep : public AuthStep {
    Q_OBJECT
   public:
    explicit OfflineStep(AccountData* data);
    virtual ~OfflineStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;
};
