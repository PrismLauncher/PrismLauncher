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

namespace Ui
{
class EditAccountDialog;
}

class EditAccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditAccountDialog(const QString &text = "", QWidget *parent = 0,
                               int flags = UsernameField | PasswordField);
    ~EditAccountDialog();

    void setUsername(const QString & user) const;
    void setPassword(const QString & pass) const;

    QString username() const;
    QString password() const;

    enum Flags
    {
        NoFlags = 0,

        //! Specifies that the dialog should have a username field.
        UsernameField,

        //! Specifies that the dialog should have a password field.
        PasswordField,
    };

private slots:
  void on_label_linkActivated(const QString &link);

private:
    Ui::EditAccountDialog *ui;
};
