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

#include "AccountListDialog.h"
#include "ui_AccountListDialog.h"

#include <logger/QsLog.h>

#include <logic/auth/AuthenticateTask.h>

#include <gui/dialogs/LoginDialog.h>
#include <gui/dialogs/ProgressDialog.h>

#include <MultiMC.h>

AccountListDialog::AccountListDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccountListDialog)
{
	ui->setupUi(this);

	m_accounts = MMC->accounts();
	ui->listView->setModel(m_accounts.get());
}

AccountListDialog::~AccountListDialog()
{
	delete ui;
}


void AccountListDialog::on_addAccountBtn_clicked()
{
	doLogin("Please log in to add your account.");
}

void AccountListDialog::on_rmAccountBtn_clicked()
{
	// TODO
}

void AccountListDialog::on_editAccountBtn_clicked()
{
	// TODO
}

void AccountListDialog::on_closedBtnBox_rejected()
{
	close();
}

void AccountListDialog::doLogin(const QString& errMsg)
{
	// TODO: We can use the login dialog for this for now, but we'll have to make something better for it eventually.
	LoginDialog loginDialog(this);
	loginDialog.exec();

	if (loginDialog.result() == QDialog::Accepted)
	{
		QString username(loginDialog.getUsername());
		QString password(loginDialog.getPassword());

		MojangAccountPtr account = MojangAccountPtr(new MojangAccount(username));

		ProgressDialog* progDialog = new ProgressDialog(this);
		m_authTask = new AuthenticateTask(account, password, progDialog);
		connect(m_authTask, SIGNAL(succeeded()), SLOT(onLoginComplete()), Qt::QueuedConnection);
		connect(m_authTask, SIGNAL(failed(QString)), SLOT(doLogin(QString)), Qt::QueuedConnection);
		progDialog->exec(m_authTask);
		//delete m_authTask;
	}
}

void AccountListDialog::onLoginComplete()
{
	// Add the authenticated account to the accounts list.
	MojangAccountPtr account = m_authTask->getMojangAccount();
	m_accounts->addAccount(account);
	//ui->listView->update();
}

