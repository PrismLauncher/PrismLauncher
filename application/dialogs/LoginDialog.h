/* Copyright 2013-2019 MultiMC Contributors
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

#include <QtWidgets/QDialog>
#include <QtCore/QEventLoop>

#include "minecraft/auth/MojangAccount.h"

namespace Ui
{
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    ~LoginDialog();

    static MojangAccountPtr newAccount(QWidget *parent, QString message);

private:
    explicit LoginDialog(QWidget *parent = 0);

    void setUserInputsEnabled(bool enable);

protected
slots:
    void accept();

    void onTaskFailed(const QString &reason);
    void onTaskSucceeded();
    void onTaskStatus(const QString &status);
    void onTaskProgress(qint64 current, qint64 total);

    void on_userTextBox_textEdited(const QString &newText);
    void on_passTextBox_textEdited(const QString &newText);

private:
    Ui::LoginDialog *ui;
    MojangAccountPtr m_account;
    std::shared_ptr<Task> m_loginTask;
};
