/* Copyright 2013-2021 MultiMC Contributors
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

#include "Launcher.h"
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
    ui->iconButton->setIcon(LAUNCHER->icons()->getIcon(InstIconKey));
    ui->instNameTextBox->setText(original->name());
    ui->instNameTextBox->setFocus();
    auto groups = LAUNCHER->instances()->getGroups().toSet();
    auto groupList = QStringList(groups.toList());
    groupList.sort(Qt::CaseInsensitive);
    groupList.removeOne("");
    groupList.push_front("");
    ui->groupBox->addItems(groupList);
    int index = groupList.indexOf(LAUNCHER->instances()->getInstanceGroup(m_original->id()));
    if(index == -1)
    {
        index = 0;
    }
    ui->groupBox->setCurrentIndex(index);
    ui->groupBox->lineEdit()->setPlaceholderText(tr("No group"));
    ui->copySavesCheckbox->setChecked(m_copySaves);
    ui->keepPlaytimeCheckbox->setChecked(m_keepPlaytime);
}

CopyInstanceDialog::~CopyInstanceDialog()
{
    delete ui;
}

void CopyInstanceDialog::updateDialogState()
{
    auto allowOK = !instName().isEmpty();
    auto OkButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    if(OkButton->isEnabled() != allowOK)
    {
        OkButton->setEnabled(allowOK);
    }
}

QString CopyInstanceDialog::instName() const
{
    auto result = ui->instNameTextBox->text().trimmed();
    if(result.size())
    {
        return result;
    }
    return QString();
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
        ui->iconButton->setIcon(LAUNCHER->icons()->getIcon(InstIconKey));
    }
}

void CopyInstanceDialog::on_instNameTextBox_textChanged(const QString &arg1)
{
    updateDialogState();
}

bool CopyInstanceDialog::shouldCopySaves() const
{
    return m_copySaves;
}

void CopyInstanceDialog::on_copySavesCheckbox_stateChanged(int state)
{
    if(state == Qt::Unchecked)
    {
        m_copySaves = false;
    }
    else if(state == Qt::Checked)
    {
        m_copySaves = true;
    }
}

bool CopyInstanceDialog::shouldKeepPlaytime() const
{
    return m_keepPlaytime;
}


void CopyInstanceDialog::on_keepPlaytimeCheckbox_stateChanged(int state)
{
    if(state == Qt::Unchecked)
    {
        m_keepPlaytime = false;
    }
    else if(state == Qt::Checked)
    {
        m_keepPlaytime = true;
    }
}
