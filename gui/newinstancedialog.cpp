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

#include "instanceloader.h"
#include "instancetypeinterface.h"

#include "instversionlist.h"
#include "instversion.h"

#include "task.h"

#include "versionselectdialog.h"
#include "taskdialog.h"

#include <QLayout>
#include <QPushButton>

NewInstanceDialog::NewInstanceDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::NewInstanceDialog)
{
	m_selectedType = NULL;
	m_selectedVersion = NULL;
	
	ui->setupUi(this);
	resize(minimumSizeHint());
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	
	loadTypeList();
}

NewInstanceDialog::~NewInstanceDialog()
{
	delete ui;
}

void NewInstanceDialog::loadTypeList()
{
	InstTypeList typeList = InstanceLoader::get().typeList();
	
	for (int i = 0; i < typeList.length(); i++)
	{
		ui->instTypeComboBox->addItem(typeList.at(i)->displayName(), typeList.at(i)->typeID());
	}
	
	updateSelectedType();
}

void NewInstanceDialog::updateSelectedType()
{
	QString typeID = ui->instTypeComboBox->itemData(ui->instTypeComboBox->currentIndex()).toString();
	
	const InstanceTypeInterface *type = InstanceLoader::get().findType(typeID);
	m_selectedType = type;
	
	updateDialogState();
	
	if (m_selectedType)
	{
		if (!m_selectedType->versionList()->isLoaded())
			loadVersionList();
	}
}

void NewInstanceDialog::updateDialogState()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_selectedType && m_selectedVersion);
	ui->btnChangeVersion->setEnabled(m_selectedType && m_selectedVersion);
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

void NewInstanceDialog::loadVersionList()
{
	if (!m_selectedType)
		return;
	
	TaskDialog *taskDlg = new TaskDialog(this);
	Task *loadTask = m_selectedType->versionList()->getLoadTask();
	loadTask->setParent(taskDlg);
	taskDlg->exec(loadTask);
	
	setSelectedVersion(m_selectedType->versionList()->getLatestStable());
}

void NewInstanceDialog::on_btnChangeVersion_clicked()
{
	if (m_selectedType)
	{
		VersionSelectDialog *vselect = new VersionSelectDialog(m_selectedType->versionList(), this);
		if (vselect->exec())
		{
			const InstVersion *version = vselect->selectedVersion();
			if (version)
				setSelectedVersion(version);
		}
	}
}

void NewInstanceDialog::on_instTypeComboBox_activated(int index)
{
	updateSelectedType();
}
