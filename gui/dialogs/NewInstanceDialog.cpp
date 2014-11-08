/* Copyright 2013-2014 MultiMC Contributors
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

#include "MultiMC.h"
#include "NewInstanceDialog.h"
#include "ui_NewInstanceDialog.h"

#include "logic/InstanceFactory.h"
#include "logic/BaseVersion.h"
#include "logic/icons/IconList.h"
#include "logic/minecraft/MinecraftVersionList.h"
#include "logic/tasks/Task.h"
#include <logic/InstanceList.h>

#include "gui/Platform.h"
#include "VersionSelectDialog.h"
#include "ProgressDialog.h"
#include "IconPickerDialog.h"

#include <QLayout>
#include <QPushButton>

NewInstanceDialog::NewInstanceDialog(QWidget *parent)
	: QDialog(parent), ui(new Ui::NewInstanceDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	resize(minimumSizeHint());
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	setSelectedVersion(MMC->minecraftlist()->getLatestStable(), true);
	InstIconKey = "infinity";
	ui->iconButton->setIcon(MMC->icons()->getIcon(InstIconKey));
	auto groups = MMC->instances()->getGroups().toSet();
	auto groupList = QStringList(groups.toList());
	groupList.sort(Qt::CaseInsensitive);
	groupList.removeOne("");
	QString oldValue = MMC->settings()->get("LastUsedGroupForNewInstance").toString();
	groupList.push_front(oldValue);
	groupList.push_front("");
	ui->groupBox->addItems(groupList);
	int index = groupList.indexOf(oldValue);
	if(index == -1)
	{
		index = 0;
	}
	ui->groupBox->setCurrentIndex(index);
	ui->groupBox->lineEdit()->setPlaceholderText(tr("No group"));
}

NewInstanceDialog::~NewInstanceDialog()
{
	delete ui;
}

void NewInstanceDialog::updateDialogState()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)
		->setEnabled(!instName().isEmpty() && m_selectedVersion);
}

void NewInstanceDialog::setSelectedVersion(BaseVersionPtr version, bool initial)
{
	m_selectedVersion = version;

	if (m_selectedVersion)
	{
		ui->versionTextBox->setText(version->name());
		if(ui->instNameTextBox->text().isEmpty() && !initial)
		{
			ui->instNameTextBox->setText(version->name());
		}
	}
	else
	{
		ui->versionTextBox->setText("");
	}

	updateDialogState();
}

QString NewInstanceDialog::instName() const
{
	return ui->instNameTextBox->text();
}

QString NewInstanceDialog::instGroup() const
{
	return ui->groupBox->currentText();
}

QString NewInstanceDialog::iconKey() const
{
	return InstIconKey;
}

BaseVersionPtr NewInstanceDialog::selectedVersion() const
{
	return m_selectedVersion;
}

void NewInstanceDialog::on_btnChangeVersion_clicked()
{
	VersionSelectDialog vselect(MMC->minecraftlist().get(), tr("Change Minecraft version"),
								this);
	vselect.exec();
	if (vselect.result() == QDialog::Accepted)
	{
		BaseVersionPtr version = vselect.selectedVersion();
		if (version)
			setSelectedVersion(version);
	}
}

void NewInstanceDialog::on_iconButton_clicked()
{
	IconPickerDialog dlg(this);
	dlg.exec(InstIconKey);

	if (dlg.result() == QDialog::Accepted)
	{
		InstIconKey = dlg.selectedIconKey;
		ui->iconButton->setIcon(MMC->icons()->getIcon(InstIconKey));
	}
}

void NewInstanceDialog::on_instNameTextBox_textChanged(const QString &arg1)
{
	updateDialogState();
}

