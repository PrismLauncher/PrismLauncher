/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QDialog>
#include <QIcon>
#include <QTimer>
#include <QNetworkReply>

#include <memory>
#include <minecraft/auth/MinecraftAccount.h>

namespace Ui
{
class ProfileSetupDialog;
}

class ProfileSetupDialog : public QDialog
{
    Q_OBJECT
public:

    explicit ProfileSetupDialog(MinecraftAccountPtr accountToSetup, QWidget *parent = 0);
    ~ProfileSetupDialog();

    enum class NameStatus
    {
        NotSet,
        Pending,
        Available,
        Exists,
        Error
    } nameStatus = NameStatus::NotSet;

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    void nameEdited(const QString &name);
    void checkFinished(
        QNetworkReply::NetworkError error,
        QByteArray data,
        QList<QNetworkReply::RawHeaderPair> headers
    );
    void startCheck();

    void setupProfileFinished(
        QNetworkReply::NetworkError error,
        QByteArray data,
        QList<QNetworkReply::RawHeaderPair> headers
    );
protected:
    void scheduleCheck(const QString &name);
    void checkName(const QString &name);
    void setNameStatus(NameStatus status, QString errorString);

    void setupProfile(const QString & profileName);

private:
    MinecraftAccountPtr m_accountToSetup;
    Ui::ProfileSetupDialog *ui;
    QIcon goodIcon;
    QIcon yellowIcon;
    QIcon badIcon;
    QAction * validityAction = nullptr;

    QString queuedCheck;

    bool isChecking = false;
    bool isWorking = false;
    QString currentCheck;

    QTimer checkStartTimer;
};

