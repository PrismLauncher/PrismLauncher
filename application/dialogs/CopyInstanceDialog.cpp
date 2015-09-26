/* Copyright 2013-2015 MultiMC Contributors
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

#include <QLayout>
#include <QPushButton>

#include "MultiMC.h"
#include "CopyInstanceDialog.h"
#include "ui_CopyInstanceDialog.h"

#include "dialogs/IconPickerDialog.h"

#include "BaseVersion.h"
#include "icons/IconList.h"
#include "tasks/Task.h"
#include "BaseInstance.h"
#include "InstanceList.h"

CopyInstanceDialog::CopyInstanceDialog(InstancePtr original, QWidget *parent)
	:QDialog(parent), ui(new Ui::CopyInstanceDialog), m_original(original)
{
	ui->setupUi(this);
	resize(minimumSizeHint());
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	InstIconKey = original->iconKey();
	ui->iconButton->setIcon(ENV.icons()->getIcon(InstIconKey));
	ui->instNameTextBox->setText(original->name());
	ui->instNameTextBox->setFocus();
	auto groups = MMC->instances()->getGroups().toSet();
	auto groupList = QStringList(groups.toList());
	groupList.sort(Qt::CaseInsensitive);
	groupList.removeOne("");
	groupList.push_front("");
	ui->groupBox->addItems(groupList);
	int index = groupList.indexOf(m_original->group());
	if(index == -1)
	{
		index = 0;
	}
	ui->groupBox->setCurrentIndex(index);
	ui->groupBox->lineEdit()->setPlaceholderText(tr("No group"));
}

CopyInstanceDialog::~CopyInstanceDialog()
{
	delete ui;
}

void CopyInstanceDialog::updateDialogState()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!instName().isEmpty());
}

QString CopyInstanceDialog::instName() const
{
	return ui->instNameTextBox->text();
}

QString CopyInstanceDialog::iconKey() const
{
	return InstIconKey;
}

QString CopyInstanceDialog::instGroup() const
{
	return ui->groupBox->currentText();
}

void CopyInstanceDialog::on_iconButton_clicked()
{
	IconPickerDialog dlg(this);
	dlg.execWithSelection(InstIconKey);

	if (dlg.result() == QDialog::Accepted)
	{
		InstIconKey = dlg.selectedIconKey;
		ui->iconButton->setIcon(ENV.icons()->getIcon(InstIconKey));
	}
}

void CopyInstanceDialog::on_instNameTextBox_textChanged(const QString &arg1)
{
	updateDialogState();
}
