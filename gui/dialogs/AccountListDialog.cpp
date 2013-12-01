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

#include <QItemSelectionModel>

#include <logger/QsLog.h>

#include <logic/auth/flows/AuthenticateTask.h>
#include <logic/net/NetJob.h>

#include <gui/dialogs/EditAccountDialog.h>
#include <gui/dialogs/ProgressDialog.h>
#include <gui/dialogs/AccountSelectDialog.h>

#include <MultiMC.h>

AccountListDialog::AccountListDialog(QWidget *parent)
	: QDialog(parent), ui(new Ui::AccountListDialog)
{
	ui->setupUi(this);

	m_accounts = MMC->accounts();
	// TODO: Make the "Active?" column show checkboxes or radio buttons.
	ui->listView->setModel(m_accounts.get());

	QItemSelectionModel* selectionModel = ui->listView->selectionModel();

	connect(selectionModel, &QItemSelectionModel::selectionChanged, 
			[this] (const QItemSelection& sel, const QItemSelection& dsel) { updateButtonStates(); });

	connect(m_accounts.get(), SIGNAL(listChanged()), SLOT(listChanged()));
	connect(m_accounts.get(), SIGNAL(activeAccountChanged()), SLOT(listChanged()));

	updateButtonStates();
}

AccountListDialog::~AccountListDialog()
{
	delete ui;
}

void AccountListDialog::listChanged()
{
	updateButtonStates();
}

void AccountListDialog::on_addAccountBtn_clicked()
{
	addAccount(tr("Please enter your Mojang or Minecraft account username and password to add your account."));
}

void AccountListDialog::on_rmAccountBtn_clicked()
{
	QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
	if (selection.size() > 0)
	{
		QModelIndex selected = selection.first();
		m_accounts->removeAccount(selected);
	}
}

void AccountListDialog::on_setDefaultBtn_clicked()
{
	QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
	if (selection.size() > 0)
	{
		QModelIndex selected = selection.first();
		MojangAccountPtr account = selected.data(MojangAccountList::PointerRole).value<MojangAccountPtr>();
		m_accounts->setActiveAccount(account->username());
	}
}

void AccountListDialog::on_noDefaultBtn_clicked()
{
	m_accounts->setActiveAccount("");
}

void AccountListDialog::on_closeBtnBox_rejected()
{
	close();
}

void AccountListDialog::updateButtonStates()
{
	// If there is no selection, disable buttons that require something selected.
	QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();

	ui->rmAccountBtn->setEnabled(selection.size() > 0);
	ui->setDefaultBtn->setEnabled(selection.size() > 0);

	ui->noDefaultBtn->setDown(m_accounts->activeAccount().get() == nullptr);
}

void AccountListDialog::addAccount(const QString& errMsg)
{
	// TODO: We can use the login dialog for this for now, but we'll have to make something better for it eventually.
	EditAccountDialog loginDialog(errMsg, this, EditAccountDialog::UsernameField | EditAccountDialog::PasswordField);
	loginDialog.exec();

	if (loginDialog.result() == QDialog::Accepted)
	{
		QString username(loginDialog.username());
		QString password(loginDialog.password());

		MojangAccountPtr account = MojangAccountPtr(new MojangAccount(username));

		ProgressDialog progDialog(this);
		AuthenticateTask authTask(account, password, &progDialog);
		if (progDialog.exec(&authTask))
		{
			// Add the authenticated account to the accounts list.
			MojangAccountPtr account = authTask.getMojangAccount();
			m_accounts->addAccount(account);

			// Grab associated player skins
			auto job = new NetJob("Player skins: " + account->username());

			for(AccountProfile profile : account->profiles())
			{
				auto meta = MMC->metacache()->resolveEntry("skins", profile.name() + ".png");
				auto action = CacheDownload::make(
					QUrl("http://skins.minecraft.net/MinecraftSkins/" + profile.name() + ".png"),
					meta);
				job->addNetAction(action);
				meta->stale = true;
			}

			job->start();
		}
	}
}
