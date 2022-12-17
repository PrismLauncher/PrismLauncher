#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>

#include "AuthFlow.h"
#include "katabasis/Globals.h"

#include <Application.h>

AuthFlow::AuthFlow(AccountData * data, QObject *parent) :
    AccountTask(data, parent)
{
}

void AuthFlow::succeed() {
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data->validity_ = Katabasis::Validity::Certain;
    changeState(
        AccountTaskState::STATE_SUCCEEDED,
        tr("Finished all authentication steps")
    );
}

void AuthFlow::executeTask() {
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep) {
        return;
    }
    changeState(AccountTaskState::STATE_WORKING, tr("Initializing"));
    nextStep();
}

void AuthFlow::nextStep() {
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_steps.size() == 0) {
        // we got to the end without an incident... assume this is all.
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep.reset();
        succeed();
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_steps.front();
    qDebug() << "AuthFlow:" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep->describe();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_steps.pop_front();
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep.get(), &AuthStep::finished, this, &AuthFlow::stepFinished);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep.get(), &AuthStep::showVerificationUriAndCode, this, &AuthFlow::showVerificationUriAndCode);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep.get(), &AuthStep::hideVerificationUriAndCode, this, &AuthFlow::hideVerificationUriAndCode);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep->perform();
}


QString AuthFlow::getStateMessage() const {
    switch (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_taskState)
    {
        case AccountTaskState::STATE_WORKING: {
            if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep) {
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentStep->describe();
            }
            else {
                return tr("Working...");
            }
        }
        default: {
            return AccountTask::getStateMessage();
        }
    }
}

void AuthFlow::stepFinished(AccountTaskState resultingState, QString message) {
    if(changeState(resultingState, message)) {
        nextStep();
    }
}
