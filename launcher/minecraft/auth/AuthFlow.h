#pragma once

#include <QImage>
#include <QList>
#include <QNetworkReply>
#include <QObject>
#include <QSet>
#include <QVector>

#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AuthStep.h"
#include "tasks/Task.h"

class AuthFlow : public Task {
    Q_OBJECT

   public:
    explicit AuthFlow(AccountData* data, bool silent = false, QObject* parent = 0);
    virtual ~AuthFlow() = default;

    void executeTask() override;

    AccountTaskState taskState() { return m_taskState; }

   signals:
    void authorizeWithBrowser(const QUrl& url);

   protected:
    void succeed();
    void nextStep();

   private slots:
    // NOTE: true -> non-terminal state, false -> terminal state
    bool changeState(AccountTaskState newState, QString reason = QString());
    void stepFinished(AccountTaskState resultingState, QString message);

   private:
    AccountTaskState m_taskState = AccountTaskState::STATE_CREATED;
    QList<AuthStep::Ptr> m_steps;
    AuthStep::Ptr m_currentStep;
    AccountData* m_data = nullptr;
};
