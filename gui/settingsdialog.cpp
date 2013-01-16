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

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

void SettingsDialog::updateCheckboxStuff()
{
	ui->minMemSpinBox->setEnabled(!(ui->compatModeCheckBox->isChecked() || 
									ui->maximizedCheckBox->isChecked()));
	ui->maxMemSpinBox->setEnabled(!(ui->compatModeCheckBox->isChecked() || 
									ui->maximizedCheckBox->isChecked()));
	
	ui->maximizedCheckBox->setEnabled(!ui->compatModeCheckBox->isChecked());
}

void SettingsDialog::on_instDirBrowseBtn_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, "Instance Directory", 
													ui->instDirTextBox->text());
	if (!dir.isEmpty())
		ui->instDirTextBox->setText(dir);
}

void SettingsDialog::on_modsDirBrowseBtn_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, "Mods Directory", 
													ui->modsDirTextBox->text());
	if (!dir.isEmpty())
		ui->modsDirTextBox->setText(dir);
}

void SettingsDialog::on_lwjglDirBrowseBtn_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, "LWJGL Directory", 
													ui->lwjglDirTextBox->text());
	if (!dir.isEmpty())
		ui->lwjglDirTextBox->setText(dir);
}

void SettingsDialog::on_compatModeCheckBox_clicked(bool checked)
{
	Q_UNUSED(checked);
	updateCheckboxStuff();
}

void SettingsDialog::on_maximizedCheckBox_clicked(bool checked)
{
	Q_UNUSED(checked);
	updateCheckboxStuff();
}
