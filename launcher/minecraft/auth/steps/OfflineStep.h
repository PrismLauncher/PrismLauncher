#pragma once
#include <QObject>

#include "minecraft/auth/AuthStep.h"

class OfflineStep : public AuthStep {
    Q_OBJECT
   public:
    explicit OfflineStep(AccountData* data);
    virtual ~OfflineStep() noexcept = default;

    void perform() override;

    QString describe() override;
};
