/* Copyright 2013-2017 MultiMC Contributors
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

#include "EditAccountDialog.h"
#include "ui_EditAccountDialog.h"
#include <DesktopServices.h>
#include <QUrl>

EditAccountDialog::EditAccountDialog(const QString &text, QWidget *parent, int flags)
	: QDialog(parent), ui(new Ui::EditAccountDialog)
{
	ui->setupUi(this);

	ui->label->setText(text);
	ui->label->setVisible(!text.isEmpty());

	ui->userTextBox->setEnabled(flags & UsernameField);
	ui->passTextBox->setEnabled(flags & PasswordField);
}

EditAccountDialog::~EditAccountDialog()
{
	delete ui;
}

void EditAccountDialog::on_label_linkActivated(const QString &link)
{
	DesktopServices::openUrl(QUrl(link));
}

void EditAccountDialog::setUsername(const QString & user) const
{
	ui->userTextBox->setText(user);
}

QString EditAccountDialog::username() const
{
	return ui->userTextBox->text();
}

void EditAccountDialog::setPassword(const QString & pass) const
{
	ui->passTextBox->setText(pass);
}

QString EditAccountDialog::password() const
{
	return ui->passTextBox->text();
}
