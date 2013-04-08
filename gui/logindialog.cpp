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

#include "logindialog.h"
#include "ui_logindialog.h"
#include "keyring.h"

LoginDialog::LoginDialog(QWidget *parent, const QString& loginErrMsg) :
	QDialog(parent),
	ui(new Ui::LoginDialog)
{
	ui->setupUi(this);
	//FIXME: translateable?
	ui->usernameTextBox->lineEdit()->setPlaceholderText(QApplication::translate("LoginDialog", "Name", 0));
	
	connect(ui->usernameTextBox, SIGNAL(currentTextChanged(QString)), this, SLOT(userTextChanged(QString)));
	connect(ui->forgetButton, SIGNAL(clicked(bool)), this, SLOT(forgetCurrentUser()));
	
	if (loginErrMsg.isEmpty())
		ui->loginErrorLabel->setVisible(false);
	else
	{
		ui->loginErrorLabel->setVisible(true);
		ui->loginErrorLabel->setText(QString("<span style=\" color:#ff0000;\">%1</span>").
								   arg(loginErrMsg));
	}
	
	resize(minimumSizeHint());
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	Keyring * k = Keyring::instance();
	QStringList accounts = k->getStoredAccounts("minecraft");
	ui->usernameTextBox->addItems(accounts);
	
	// TODO: restore last selected account here, if applicable
	
	int index = ui->usernameTextBox->currentIndex();
	if(index != -1)
	{
		ui->passwordTextBox->setFocus(Qt::OtherFocusReason);
	}
}

LoginDialog::~LoginDialog()
{
	delete ui;
}

QString LoginDialog::getUsername() const
{
	return ui->usernameTextBox->currentText();
}

QString LoginDialog::getPassword() const
{
	return ui->passwordTextBox->text();
}

void LoginDialog::forgetCurrentUser()
{
	Keyring * k = Keyring::instance();
	QString acct = ui->usernameTextBox->currentText();
	k->removeStoredAccount("minecraft", acct);
	ui->passwordTextBox->clear();
	int index = ui->usernameTextBox->findText(acct);
	if(index != -1)
		ui->usernameTextBox->removeItem(index);
}

void LoginDialog::userTextChanged ( const QString& user )
{
	Keyring * k = Keyring::instance();
	QString acct = ui->usernameTextBox->currentText();
	QString passwd = k->getPassword("minecraft",acct);
	ui->passwordTextBox->setText(passwd);
}


void LoginDialog::accept()
{
	bool saveName = ui->rememberUsernameCheckbox->isChecked();
	bool savePass = ui->rememberPasswordCheckbox->isChecked();
	if(saveName)
	{
		Keyring * k = Keyring::instance();
		if(savePass)
		{
			k->storePassword("minecraft",getUsername(),getPassword());
		}
		else
		{
			k->storePassword("minecraft",getUsername(),QString());
		}
	}
	QDialog::accept();
}
