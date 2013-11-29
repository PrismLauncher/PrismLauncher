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

#include "AccountSelectDialog.h"
#include "ui_AccountSelectDialog.h"

#include <QItemSelectionModel>

#include <logger/QsLog.h>

#include <logic/auth/AuthenticateTask.h>

#include <gui/dialogs/ProgressDialog.h>

#include <MultiMC.h>

AccountSelectDialog::AccountSelectDialog(const QString& message, int flags, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AccountSelectDialog)
{
	ui->setupUi(this);

	m_accounts = MMC->accounts();
	ui->listView->setModel(m_accounts.get());
	ui->listView->hideColumn(MojangAccountList::ActiveColumn);

	// Set the message label.
	ui->msgLabel->setVisible(!message.isEmpty());
	ui->msgLabel->setText(message);

	// Flags...
	ui->globalDefaultCheck->setVisible(flags & GlobalDefaultCheckbox);
	ui->instDefaultCheck->setVisible(flags & InstanceDefaultCheckbox);
	QLOG_DEBUG() << flags;

	// Select the first entry in the list.
	ui->listView->setCurrentIndex(ui->listView->model()->index(0, 0));
}

AccountSelectDialog::~AccountSelectDialog()
{
	delete ui;
}

MojangAccountPtr AccountSelectDialog::selectedAccount() const
{
	return m_selected;
}

bool AccountSelectDialog::useAsGlobalDefault() const
{
	return ui->globalDefaultCheck->isChecked();
}

bool AccountSelectDialog::useAsInstDefaullt() const
{
	return ui->instDefaultCheck->isChecked();
}

void AccountSelectDialog::on_buttonBox_accepted()
{
	QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
	if (selection.size() > 0)
	{
		QModelIndex selected = selection.first();
		MojangAccountPtr account = selected.data(MojangAccountList::PointerRole).value<MojangAccountPtr>();
		m_selected = account;
	}
	close();
}

void AccountSelectDialog::on_buttonBox_rejected()
{
	close();
}

