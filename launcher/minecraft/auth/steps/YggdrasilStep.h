#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"

class Yggdrasil;

class YggdrasilStep : public AuthStep {
    Q_OBJECT

public:
    explicit YggdrasilStep(AccountData *data, QString password);
    virtual ~YggdrasilStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

private slots:
    void onAuthSucceeded();
    void onAuthFailed();

private:
    Yggdrasil *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_yggdrasil = nullptr;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_password;
};
