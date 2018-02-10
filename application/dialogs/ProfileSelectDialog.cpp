/* Copyright 2013-2018 MultiMC Contributors
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

#include "ProfileSelectDialog.h"
#include <SkinUtils.h>
#include "ui_ProfileSelectDialog.h"

#include <QItemSelectionModel>

#include <QDebug>

#include <dialogs/ProgressDialog.h>

#include <MultiMC.h>

ProfileSelectDialog::ProfileSelectDialog(const QString &message, int flags, QWidget *parent)
	: QDialog(parent), ui(new Ui::ProfileSelectDialog)
{
	ui->setupUi(this);

	m_accounts = MMC->accounts();
	auto view = ui->listView;
	//view->setModel(m_accounts.get());
	//view->hideColumn(MojangAccountList::ActiveColumn);
	view->setColumnCount(1);
	view->setRootIsDecorated(false);
	if(QTreeWidgetItem* header = view->headerItem())
	{
		header->setText(0, tr("Name"));
	}
	else
	{
		view->setHeaderLabel(tr("Name"));
	}
	QList <QTreeWidgetItem *> items;
	for (int i = 0; i < m_accounts->count(); i++)
	{
		MojangAccountPtr account = m_accounts->at(i);
		for (auto profile : account->profiles())
		{
			auto profileLabel = profile.name;
			if(account->isInUse())
			{
				profileLabel += tr(" (in use)");
			}
			auto item = new QTreeWidgetItem(view);
			item->setText(0, profileLabel);
			item->setIcon(0, SkinUtils::getFaceFromCache(profile.id));
			item->setData(0, MojangAccountList::PointerRole, QVariant::fromValue(account));
			items.append(item);
		}
	}
	view->addTopLevelItems(items);

	// Set the message label.
	ui->msgLabel->setVisible(!message.isEmpty());
	ui->msgLabel->setText(message);

	// Flags...
	ui->globalDefaultCheck->setVisible(flags & GlobalDefaultCheckbox);
	ui->instDefaultCheck->setVisible(flags & InstanceDefaultCheckbox);
	qDebug() << flags;

	// Select the first entry in the list.
	ui->listView->setCurrentIndex(ui->listView->model()->index(0, 0));

	connect(ui->listView, SIGNAL(doubleClicked(QModelIndex)), SLOT(on_buttonBox_accepted()));
}

ProfileSelectDialog::~ProfileSelectDialog()
{
	delete ui;
}

MojangAccountPtr ProfileSelectDialog::selectedAccount() const
{
	return m_selected;
}

bool ProfileSelectDialog::useAsGlobalDefault() const
{
	return ui->globalDefaultCheck->isChecked();
}

bool ProfileSelectDialog::useAsInstDefaullt() const
{
	return ui->instDefaultCheck->isChecked();
}

void ProfileSelectDialog::on_buttonBox_accepted()
{
	QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
	if (selection.size() > 0)
	{
		QModelIndex selected = selection.first();
		m_selected = selected.data(MojangAccountList::PointerRole).value<MojangAccountPtr>();
	}
	close();
}

void ProfileSelectDialog::on_buttonBox_rejected()
{
	close();
}
