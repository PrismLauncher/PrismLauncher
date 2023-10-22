#pragma once

#include <QImage>
#include <QList>
#include <QNetworkReply>
#include <QObject>
#include <QSet>
#include <QVector>

#include <katabasis/DeviceFlow.h>

#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AccountTask.h"
#include "minecraft/auth/AuthStep.h"

class AuthFlow : public AccountTask {
    Q_OBJECT

   public:
    explicit AuthFlow(AccountData* data, QObject* parent = 0);

    Katabasis::Validity validity() { return m_data->validity_; };

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
    QList<AuthStep::Ptr> m_steps;
    AuthStep::Ptr m_currentStep;
};
