#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "AuthFlow.h"

#include <Application.h>

AuthFlow::AuthFlow(AccountData* data, QObject* parent) : Task(parent), m_data(data)
{
    changeState(AccountTaskState::STATE_CREATED);
}

void AuthFlow::succeed()
{
    m_data->validity_ = Katabasis::Validity::Certain;
    changeState(AccountTaskState::STATE_SUCCEEDED, tr("Finished all authentication steps"));
}

void AuthFlow::executeTask()
{
    if (m_currentStep) {
        return;
    }
    changeState(AccountTaskState::STATE_WORKING, tr("Initializing"));
    nextStep();
}

void AuthFlow::nextStep()
{
    if (m_steps.size() == 0) {
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

QString AuthFlow::getStateMessage() const
{
    switch (m_taskState) {
        case AccountTaskState::STATE_CREATED:
            return "Waiting...";
        case AccountTaskState::STATE_WORKING:
            if (m_currentStep)
                return m_currentStep->describe();
            return tr("Working...");
        case AccountTaskState::STATE_SUCCEEDED:
            return tr("Authentication task succeeded.");
        case AccountTaskState::STATE_OFFLINE:
            return tr("Failed to contact the authentication server.");
        case AccountTaskState::STATE_DISABLED:
            return tr("Client ID has changed. New session needs to be created.");
        case AccountTaskState::STATE_FAILED_SOFT:
            return tr("Encountered an error during authentication.");
        case AccountTaskState::STATE_FAILED_HARD:
            return tr("Failed to authenticate. The session has expired.");
        case AccountTaskState::STATE_FAILED_GONE:
            return tr("Failed to authenticate. The account no longer exists.");
        default:
            return tr("...");
    }
}

void AuthFlow::stepFinished(AccountTaskState resultingState, QString message)
{
    if (changeState(resultingState, message)) {
        nextStep();
    }
}

bool AuthFlow::changeState(AccountTaskState newState, QString reason)
{
    m_taskState = newState;
    // FIXME: virtual method invoked in constructor.
    // We want that behavior, but maybe make it less weird?
    setStatus(getStateMessage());
    switch (newState) {
        case AccountTaskState::STATE_CREATED: {
            m_data->errorString.clear();
            return true;
        }
        case AccountTaskState::STATE_WORKING: {
            m_data->accountState = AccountState::Working;
            return true;
        }
        case AccountTaskState::STATE_SUCCEEDED: {
            m_data->accountState = AccountState::Online;
            emitSucceeded();
            return false;
        }
        case AccountTaskState::STATE_OFFLINE: {
            m_data->errorString = reason;
            m_data->accountState = AccountState::Offline;
            emitFailed(reason);
            return false;
        }
        case AccountTaskState::STATE_DISABLED: {
            m_data->errorString = reason;
            m_data->accountState = AccountState::Disabled;
            emitFailed(reason);
            return false;
        }
        case AccountTaskState::STATE_FAILED_SOFT: {
            m_data->errorString = reason;
            m_data->accountState = AccountState::Errored;
            emitFailed(reason);
            return false;
        }
        case AccountTaskState::STATE_FAILED_HARD: {
            m_data->errorString = reason;
            m_data->accountState = AccountState::Expired;
            emitFailed(reason);
            return false;
        }
        case AccountTaskState::STATE_FAILED_GONE: {
            m_data->errorString = reason;
            m_data->accountState = AccountState::Gone;
            emitFailed(reason);
            return false;
        }
        default: {
            QString error = tr("Unknown account task state: %1").arg(int(newState));
            m_data->accountState = AccountState::Errored;
            emitFailed(error);
            return false;
        }
    }
}