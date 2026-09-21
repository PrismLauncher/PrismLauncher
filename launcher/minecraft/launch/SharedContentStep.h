// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "launch/LaunchStep.h"

class SharedContentStep : public LaunchStep {
    Q_OBJECT

   public:
    explicit SharedContentStep(LaunchTask* parent);

    void executeTask() override;
    void finalize() override;
    bool canAbort() const override { return false; }

   private:
    bool m_prepared = false;
};
