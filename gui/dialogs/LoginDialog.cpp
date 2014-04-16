/* Copyright 2014 MultiMC Contributors
 *
 * Authors:
 *      Taeyeon Mori <orochimarufan.x3@gmail.com>
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

#include "LoginDialog.h"
#include "ui_LoginDialog.h"

#include "logic/auth/YggdrasilTask.h"

#include <QtWidgets/QPushButton>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent), ui(new Ui::LoginDialog)
{
	ui->setupUi(this);
	ui->progressBar->setVisible(false);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	setAttribute(Qt::WA_ShowModal);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

LoginDialog::~LoginDialog()
{
	delete ui;
}

// Stage 1: User interaction
void LoginDialog::accept()
{
	setResult(Accepted);
	loop.quit();
}

void LoginDialog::reject()
{
	setResult(Rejected);
	loop.quit();
}

void LoginDialog::setUserInputsEnabled(bool enable)
{
	ui->userTextBox->setEnabled(enable);
	ui->passTextBox->setEnabled(enable);
	ui->buttonBox->setEnabled(enable);
}

// Enable the OK button only when both textboxes contain something.
void LoginDialog::on_userTextBox_textEdited(QString newText)
{
	ui->buttonBox->button(QDialogButtonBox::Ok)
		->setEnabled(!newText.isEmpty() && !ui->passTextBox->text().isEmpty());
}

void LoginDialog::on_passTextBox_textEdited(QString newText)
{
	ui->buttonBox->button(QDialogButtonBox::Ok)
		->setEnabled(!newText.isEmpty() && !ui->userTextBox->text().isEmpty());
}

// Stage 2: Task interaction
void LoginDialog::onTaskFailed(QString failure)
{
	// Set message
	ui->label->setText("<span style='color:red'>" + failure + "</span>");

	// Return
	setResult(Rejected);
	loop.quit();
}

void LoginDialog::onTaskSucceeded()
{
	setResult(Accepted);
	loop.quit();
}

void LoginDialog::onTaskStatus(QString status)
{
	ui->label->setText(status);
}

void LoginDialog::onTaskProgress(qint64 current, qint64 total)
{
	ui->progressBar->setMaximum(total);
	ui->progressBar->setValue(current);
}

// Public interface
MojangAccountPtr LoginDialog::newAccount(QWidget *parent, QString msg)
{
	LoginDialog dlg(parent);
	dlg.show();
	dlg.ui->label->setText(msg);

	while (1)
	{
		// Show dialog
		dlg.loop.exec();

		// Close if cancel was clicked
		if (dlg.result() == Rejected)
			return nullptr;

		// Read values
		QString username(dlg.ui->userTextBox->text());
		QString password(dlg.ui->passTextBox->text());

		// disable inputs
		dlg.setUserInputsEnabled(false);
		dlg.ui->progressBar->setVisible(true);

		// Start login process
		MojangAccountPtr account = MojangAccount::createFromUsername(username);
		auto task = account->login(nullptr, password);

		// show progess
		connect(task.get(), &ProgressProvider::failed, &dlg, &LoginDialog::onTaskFailed);
		connect(task.get(), &ProgressProvider::succeeded, &dlg, &LoginDialog::onTaskSucceeded);
		connect(task.get(), &ProgressProvider::status, &dlg, &LoginDialog::onTaskStatus);
		connect(task.get(), &ProgressProvider::progress, &dlg, &LoginDialog::onTaskProgress);

		// Start task
		if (!task->isRunning())
			task->start();
		if (task->isRunning())
			dlg.loop.exec();

		// Be done
		if (dlg.result() == Accepted)
			return account;

		// Otherwise, re-enable user inputs and begin anew
		dlg.setUserInputsEnabled(true);
		dlg.ui->progressBar->setVisible(false);
	}
}
