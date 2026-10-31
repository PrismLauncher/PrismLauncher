#pragma once

#include <QImage>
#include <QList>
#include <QObject>
#include <QSet>

#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AuthStep.h"
#include "tasks/Task.h"

class AuthFlow : public Task {
    Q_OBJECT

   public:
    enum class Action : std::uint8_t { Refresh, Login, DeviceCode };

    explicit AuthFlow(AccountData* data, Action action = Action::Refresh);
    ~AuthFlow() override = default;

    void executeTask() override;

    AccountTaskState taskState() { return m_taskState; }

   public slots:
    bool abort() override;

   signals:
    void authorizeWithBrowser(const QUrl& url);
    void authorizeWithBrowserWithExtra(const QUrl& verificationUrl, const QString& code, const QUrl& completeVerificationUrl);

   protected:
    void succeed();
    void nextStep();

   private slots:
    // NOTE: true -> non-terminal state, false -> terminal state
    bool changeState(AccountTaskState newState, const QString& reason = {});
    void stepFinished(AccountTaskState resultingState, QString message);

   private:
    AccountTaskState m_taskState = AccountTaskState::STATE_CREATED;
    QList<AuthStep::Ptr> m_steps;
    AuthStep::Ptr m_currentStep;
    AccountData* m_data = nullptr;
};
