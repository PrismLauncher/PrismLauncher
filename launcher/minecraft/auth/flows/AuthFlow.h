#pragma once

#include <QObject>
#include <QList>
#include <QVector>
#include <QSet>
#include <QNetworkReply>
#include <QImage>

#include <katabasis/DeviceFlow.h>

#include "minecraft/auth/Yggdrasil.h"
#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AccountTask.h"
#include "minecraft/auth/AuthStep.h"

class AuthFlow : public AccountTask
{
    Q_OBJECT

public:
    explicit AuthFlow(AccountData * data, QObject *parent = 0);

    Katabasis::Validity validity() {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data->validity_;
    };

    QString getStateMessage() const override;

    void executeTask() override;

signals:
    void activityChanged(Katabasis::Activity activity);

private slots:
    void stepFinished(AccountTaskState resultingState, QString message);

protected:
    void succeed();
    void nextStep();

protected:
    QList<AuthStep::Ptr> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_steps;
    AuthStep::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep;
};
