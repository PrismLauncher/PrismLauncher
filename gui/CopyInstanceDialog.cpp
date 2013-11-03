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

#include <MultiMC.h>
#include "CopyInstanceDialog.h"
#include "ui_CopyInstanceDialog.h"

#include "logic/InstanceFactory.h"
#include "logic/BaseVersion.h"
#include "logic/lists/IconList.h"
#include "logic/lists/MinecraftVersionList.h"
#include "logic/tasks/Task.h"
#include <logic/BaseInstance.h>

#include "gui/platform.h"
#include "versionselectdialog.h"
#include "ProgressDialog.h"
#include "IconPickerDialog.h"

#include <QLayout>
#include <QPushButton>

CopyInstanceDialog::CopyInstanceDialog(BaseInstance *original, QWidget *parent)
	: m_original(original), QDialog(parent), ui(new Ui::CopyInstanceDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	resize(minimumSizeHint());
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	InstIconKey = original->iconKey();
	ui->iconButton->setIcon(MMC->icons()->getIcon(InstIconKey));
	ui->instNameTextBox->setText(original->name());
	ui->instNameTextBox->setFocus();
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

void CopyInstanceDialog::on_iconButton_clicked()
{
	IconPickerDialog dlg(this);
	dlg.exec(InstIconKey);

	if (dlg.result() == QDialog::Accepted)
	{
		InstIconKey = dlg.selectedIconKey;
		ui->iconButton->setIcon(MMC->icons()->getIcon(InstIconKey));
	}
}

void CopyInstanceDialog::on_instNameTextBox_textChanged(const QString &arg1)
{
	updateDialogState();
}
