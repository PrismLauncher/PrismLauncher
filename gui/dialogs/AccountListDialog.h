/* Copyright 2013 MultiMC Contributors
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

#include "logic/lists/MojangAccountList.h"

namespace Ui {
class AccountListDialog;
}

class AuthenticateTask;

class AccountListDialog : public QDialog
{
Q_OBJECT
public:
	explicit AccountListDialog(QWidget *parent = 0);
	~AccountListDialog();

public
slots:
	void on_addAccountBtn_clicked();

	void on_rmAccountBtn_clicked();

	void on_editAccountBtn_clicked();

	// This will be sent when the "close" button is clicked.
	void on_closedBtnBox_rejected();

protected:
	// Temporarily putting this here...
	MojangAccountList m_accounts;

	AuthenticateTask* m_authTask;

protected
slots:
	void doLogin(const QString& errMsg="");
	void onLoginComplete();

private:
	Ui::AccountListDialog *ui;
};

