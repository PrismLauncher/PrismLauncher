#pragma once

#include <launch/LaunchStep.h>
#include <launch/LaunchTask.h>

class VerifyJavaInstall : public LaunchStep {
    Q_OBJECT

public:
    explicit VerifyJavaInstall(LaunchTask *parent) : LaunchStep(parent) {
    };
    ~VerifyJavaInstall() override = default;

    void executeTask() override;
    bool canAbort() const override {
        return false;
    }
};
