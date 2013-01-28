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

#include "data/appsettings.h"

#include <QFileDialog>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);
	
	loadSettings(settings);
	updateCheckboxStuff();
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

void SettingsDialog::updateCheckboxStuff()
{
	ui->windowWidthSpinBox->setEnabled(!(ui->compatModeCheckBox->isChecked() || 
										 ui->maximizedCheckBox->isChecked()));
	ui->windowHeightSpinBox->setEnabled(!(ui->compatModeCheckBox->isChecked() || 
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

void SettingsDialog::on_buttonBox_accepted()
{
	applySettings(settings);
}

void SettingsDialog::applySettings(SettingsBase *s)
{
	// Special cases
	
	// Warn about dev builds.
	if (!ui->devBuildsCheckBox->isChecked())
	{
		s->setUseDevBuilds(false);
	}
	else if (!s->getUseDevBuilds())
	{
		int response = QMessageBox::question(this, "Development builds", 
											 "Development builds contain experimental features "
											 "and may be unstable. Are you sure you want to enable them?");
		if (response == QMessageBox::Yes)
		{
			s->setUseDevBuilds(true);
		}
	}
	
	
	// Updates
	s->setAutoUpdate(ui->autoUpdateCheckBox->isChecked());
	
	// Folders
	// TODO: Offer to move instances to new instance folder.
	s->setInstanceDir(ui->instDirTextBox->text());
	s->setCentralModsDir(ui->modsDirTextBox->text());
	s->setLWJGLDir(ui->lwjglDirTextBox->text());
	
	// Console
	s->setShowConsole(ui->showConsoleCheck->isChecked());
	s->setAutoCloseConsole(ui->autoCloseConsoleCheck->isChecked());
	
	// Window Size
	s->setLaunchCompatMode(ui->compatModeCheckBox->isChecked());
	s->setLaunchMaximized(ui->maximizedCheckBox->isChecked());
	s->setMinecraftWinWidth(ui->windowWidthSpinBox->value());
	s->setMinecraftWinHeight(ui->windowHeightSpinBox->value());
	
	// Auto Login
	s->setAutoLogin(ui->autoLoginCheckBox->isChecked());
	
	// Memory
	s->setMinMemAlloc(ui->minMemSpinBox->value());
	s->setMaxMemAlloc(ui->maxMemSpinBox->value());
	
	// Java Settings
	s->setJavaPath(ui->javaPathTextBox->text());
	s->setJvmArgs(ui->jvmArgsTextBox->text());
	
	// Custom Commands
	s->setPreLaunchCommand(ui->preLaunchCmdTextBox->text());
	s->setPostExitCommand(ui->postExitCmdTextBox->text());
}

void SettingsDialog::loadSettings(SettingsBase *s)
{
	// Updates
	ui->autoUpdateCheckBox->setChecked(s->getAutoUpdate());
	ui->devBuildsCheckBox->setChecked(s->getUseDevBuilds());
	
	// Folders
	ui->instDirTextBox->setText(s->getInstanceDir());
	ui->modsDirTextBox->setText(s->getCentralModsDir());
	ui->lwjglDirTextBox->setText(s->getLWJGLDir());
	
	// Console
	ui->showConsoleCheck->setChecked(s->getShowConsole());
	ui->autoCloseConsoleCheck->setChecked(s->getAutoCloseConsole());
	
	// Window Size
	ui->compatModeCheckBox->setChecked(s->getLaunchCompatMode());
	ui->maximizedCheckBox->setChecked(s->getLaunchMaximized());
	ui->windowWidthSpinBox->setValue(s->getMinecraftWinWidth());
	ui->windowHeightSpinBox->setValue(s->getMinecraftWinHeight());
	
	// Auto Login
	ui->autoLoginCheckBox->setChecked(s->getAutoLogin());
	
	// Memory
	ui->minMemSpinBox->setValue(s->getMinMemAlloc());
	ui->maxMemSpinBox->setValue(s->getMaxMemAlloc());
	
	// Java Settings
	ui->javaPathTextBox->setText(s->getJavaPath());
	ui->jvmArgsTextBox->setText(s->getJvmArgs());
	
	// Custom Commands
	ui->preLaunchCmdTextBox->setText(s->getPreLaunchCommand());
	ui->postExitCmdTextBox->setText(s->getPostExitCommand());
}
