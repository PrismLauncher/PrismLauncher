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
    m_data->validity_ = Katabasis::Validity::Certain;
    changeState(
        AccountTaskState::STATE_SUCCEEDED,
        tr("Finished all authentication steps")
    );
}

void AuthFlow::executeTask() {
    if(m_currentStep) {
        return;
    }
    changeState(AccountTaskState::STATE_WORKING, tr("Initializing"));
    nextStep();
}

void AuthFlow::nextStep() {
    if(m_steps.size() == 0) {
        // we got to the end without an incident... assume this is all.
        m_currentStep.reset();
        succeed();
        return;
    }
    m_currentStep = m_steps.front();
    qDebug() << "AuthFlow:" << m_currentStep->describe();
    m_steps.pop_front();
    connect(m_currentStep.get(), &AuthStep::finished, this, &AuthFlow::stepFinished);
    connect(m_currentStep.get(), &AuthStep::showVerificationUriAndCode, this, &AuthFlow::showVerificationUriAndCode);
    connect(m_currentStep.get(), &AuthStep::hideVerificationUriAndCode, this, &AuthFlow::hideVerificationUriAndCode);

    m_currentStep->perform();
}


QString AuthFlow::getStateMessage() const {
    switch (m_taskState)
    {
        case AccountTaskState::STATE_WORKING: {
            if(m_currentStep) {
                return m_currentStep->describe();
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
