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

#include "newinstancedialog.h"
#include "ui_newinstancedialog.h"

#include "InstanceFactory.h"

#include <lists/MinecraftVersionList.h>
#include "InstanceVersion.h"

#include "tasks/Task.h"
#include <IconListModel.h>

#include "versionselectdialog.h"
#include "taskdialog.h"
#include "IconPickerDialog.h"

#include <QLayout>
#include <QPushButton>



NewInstanceDialog::NewInstanceDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::NewInstanceDialog)
{
	m_selectedVersion = NULL;
	
	ui->setupUi(this);
	resize(minimumSizeHint());
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	/*
	if (!MinecraftVersionList::getMainList().isLoaded())
	{
		TaskDialog *taskDlg = new TaskDialog(this);
		Task *loadTask = MinecraftVersionList::getMainList().getLoadTask();
		loadTask->setParent(taskDlg);
		taskDlg->exec(loadTask);
	}
	*/
	setSelectedVersion(MinecraftVersionList::getMainList().getLatestStable());
	InstIconKey = "infinity";
	IconList * list = IconList::instance();
	ui->iconButton->setIcon(list->getIcon(InstIconKey));
}

NewInstanceDialog::~NewInstanceDialog()
{
	delete ui;
}

void NewInstanceDialog::updateDialogState()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
				!instName().isEmpty() && m_selectedVersion);
}

void NewInstanceDialog::setSelectedVersion(const InstVersion *version)
{
	m_selectedVersion = version;
	
	if (m_selectedVersion)
	{
		ui->versionTextBox->setText(version->name());
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

QString NewInstanceDialog::iconKey() const
{
	return InstIconKey;
}

const InstVersion *NewInstanceDialog::selectedVersion() const
{
	return m_selectedVersion;
}

void NewInstanceDialog::on_btnChangeVersion_clicked()
{
	VersionSelectDialog vselect(&MinecraftVersionList::getMainList(), this);
	vselect.exec();
	if (vselect.result() == QDialog::Accepted)
	{
		const InstVersion *version = vselect.selectedVersion();
		if (version)
			setSelectedVersion(version);
	}
}

void NewInstanceDialog::on_iconButton_clicked()
{
	IconPickerDialog dlg;
	dlg.exec(InstIconKey);
	
	if(dlg.result() == QDialog::Accepted)
	{
		IconList * list = IconList::instance();
		InstIconKey = dlg.selectedIconKey;
		ui->iconButton->setIcon(list->getIcon(InstIconKey));
	}
}

void NewInstanceDialog::on_instNameTextBox_textChanged(const QString &arg1)
{
	updateDialogState();
}
