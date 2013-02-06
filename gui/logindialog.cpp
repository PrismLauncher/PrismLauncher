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

LoginDialog::LoginDialog(QWidget *parent, const QString& loginErrMsg) :
	QDialog(parent),
	ui(new Ui::LoginDialog)
{
	ui->setupUi(this);
	
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
}

LoginDialog::~LoginDialog()
{
	delete ui;
}

QString LoginDialog::getUsername() const
{
	return ui->usernameTextBox->text();
}

QString LoginDialog::getPassword() const
{
	return ui->passwordTextBox->text();
}
