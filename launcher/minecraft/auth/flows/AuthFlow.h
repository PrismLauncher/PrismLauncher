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
    explicit AuthFlow(AccountData* data, QObject* parent = 0);
    virtual ~AuthFlow() = default;

    Katabasis::Validity validity() { return m_data->validity_; };

    void executeTask() override;

    AccountTaskState taskState() { return m_taskState; }

   signals:
    void activityChanged(Katabasis::Activity activity);

   protected:
    void succeed();
    void nextStep();

   protected slots:
    // NOTE: true -> non-terminal state, false -> terminal state
    bool changeState(AccountTaskState newState, QString reason = QString());

   private slots:
    void stepFinished(AccountTaskState resultingState, QString message);

   protected:
    AccountTaskState m_taskState = AccountTaskState::STATE_CREATED;
    QList<AuthStep::Ptr> m_steps;
    AuthStep::Ptr m_currentStep;
    AccountData* m_data = nullptr;
};
