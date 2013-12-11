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

#include "MultiMC.h"

#include "gui/dialogs/SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "gui/Platform.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/CustomMessageBox.h"

#include "logic/JavaUtils.h"
#include "logic/NagUtils.h"
#include "logic/lists/JavaVersionList.h"
#include <logic/JavaChecker.h>

#include <settingsobject.h>
#include <pathutils.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	ui->sortingModeGroup->setId(ui->sortByNameBtn, Sort_Name);
	ui->sortingModeGroup->setId(ui->sortLastLaunchedBtn, Sort_LastLaunch);

	loadSettings(MMC->settings().get());
	updateCheckboxStuff();
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}
void SettingsDialog::showEvent(QShowEvent *ev)
{
	QDialog::showEvent(ev);
	adjustSize();
}

void SettingsDialog::updateCheckboxStuff()
{
	ui->windowWidthSpinBox->setEnabled(!ui->maximizedCheckBox->isChecked());
	ui->windowHeightSpinBox->setEnabled(!ui->maximizedCheckBox->isChecked());
}

void SettingsDialog::on_instDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Instance Directory"),
														ui->instDirTextBox->text());
	QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		ui->instDirTextBox->setText(cooked_dir);
	}
}

void SettingsDialog::on_modsDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Mods Directory"),
														ui->modsDirTextBox->text());
	QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		ui->modsDirTextBox->setText(cooked_dir);
	}
}

void SettingsDialog::on_lwjglDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(this, tr("LWJGL Directory"),
														ui->lwjglDirTextBox->text());
	QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		ui->lwjglDirTextBox->setText(cooked_dir);
	}
}

void SettingsDialog::on_maximizedCheckBox_clicked(bool checked)
{
	Q_UNUSED(checked);
	updateCheckboxStuff();
}

void SettingsDialog::on_buttonBox_accepted()
{
	applySettings(MMC->settings().get());
}

void SettingsDialog::applySettings(SettingsObject *s)
{
	// Special cases

	// Warn about dev builds.
	if (!ui->devBuildsCheckBox->isChecked())
	{
		s->set("UseDevBuilds", false);
	}
	else if (!s->get("UseDevBuilds").toBool())
	{
		auto response = CustomMessageBox::selectable(
			this, tr("Development builds"),
			tr("Development builds contain experimental features "
			   "and may be unstable. Are you sure you want to enable them?"),
			QMessageBox::Question, QMessageBox::Yes | QMessageBox::No)->exec();
		if (response == QMessageBox::Yes)
		{
			s->set("UseDevBuilds", true);
		}
	}

	// Updates
	s->set("AutoUpdate", ui->autoUpdateCheckBox->isChecked());

	// Folders
	// TODO: Offer to move instances to new instance folder.
	s->set("InstanceDir", ui->instDirTextBox->text());
	s->set("CentralModsDir", ui->modsDirTextBox->text());
	s->set("LWJGLDir", ui->lwjglDirTextBox->text());

	// Console
	s->set("ShowConsole", ui->showConsoleCheck->isChecked());
	s->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());

	// Window Size
	s->set("LaunchMaximized", ui->maximizedCheckBox->isChecked());
	s->set("MinecraftWinWidth", ui->windowWidthSpinBox->value());
	s->set("MinecraftWinHeight", ui->windowHeightSpinBox->value());

	// Auto Login
	s->set("AutoLogin", ui->autoLoginCheckBox->isChecked());

	// Memory
	s->set("MinMemAlloc", ui->minMemSpinBox->value());
	s->set("MaxMemAlloc", ui->maxMemSpinBox->value());
	s->set("PermGen", ui->permGenSpinBox->value());

	// Java Settings
	s->set("JavaPath", ui->javaPathTextBox->text());
	s->set("JvmArgs", ui->jvmArgsTextBox->text());
	NagUtils::checkJVMArgs(s->get("JvmArgs").toString(), this->parentWidget());

	// Custom Commands
	s->set("PreLaunchCommand", ui->preLaunchCmdTextBox->text());
	s->set("PostExitCommand", ui->postExitCmdTextBox->text());

	auto sortMode = (InstSortMode)ui->sortingModeGroup->checkedId();
	switch (sortMode)
	{
	case Sort_LastLaunch:
		s->set("InstSortMode", "LastLaunch");
		break;
	case Sort_Name:
	default:
		s->set("InstSortMode", "Name");
		break;
	}

	s->set("PostExitCommand", ui->postExitCmdTextBox->text());
}

void SettingsDialog::loadSettings(SettingsObject *s)
{
	// Updates
	ui->autoUpdateCheckBox->setChecked(s->get("AutoUpdate").toBool());
	ui->devBuildsCheckBox->setChecked(s->get("UseDevBuilds").toBool());

	// Folders
	ui->instDirTextBox->setText(s->get("InstanceDir").toString());
	ui->modsDirTextBox->setText(s->get("CentralModsDir").toString());
	ui->lwjglDirTextBox->setText(s->get("LWJGLDir").toString());

	// Console
	ui->showConsoleCheck->setChecked(s->get("ShowConsole").toBool());
	ui->autoCloseConsoleCheck->setChecked(s->get("AutoCloseConsole").toBool());

	// Window Size
	ui->maximizedCheckBox->setChecked(s->get("LaunchMaximized").toBool());
	ui->windowWidthSpinBox->setValue(s->get("MinecraftWinWidth").toInt());
	ui->windowHeightSpinBox->setValue(s->get("MinecraftWinHeight").toInt());

	// Auto Login
	ui->autoLoginCheckBox->setChecked(s->get("AutoLogin").toBool());

	// Memory
	ui->minMemSpinBox->setValue(s->get("MinMemAlloc").toInt());
	ui->maxMemSpinBox->setValue(s->get("MaxMemAlloc").toInt());
	ui->permGenSpinBox->setValue(s->get("PermGen").toInt());

	QString sortMode = s->get("InstSortMode").toString();

	if (sortMode == "LastLaunch")
	{
		ui->sortLastLaunchedBtn->setChecked(true);
	}
	else
	{
		ui->sortByNameBtn->setChecked(true);
	}

	// Java Settings
	ui->javaPathTextBox->setText(s->get("JavaPath").toString());
	ui->jvmArgsTextBox->setText(s->get("JvmArgs").toString());

	// Custom Commands
	ui->preLaunchCmdTextBox->setText(s->get("PreLaunchCommand").toString());
	ui->postExitCmdTextBox->setText(s->get("PostExitCommand").toString());
}

void SettingsDialog::on_javaDetectBtn_clicked()
{
	JavaVersionPtr java;

	VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this, true);
	vselect.setResizeOn(2);
	vselect.exec();

	if (vselect.result() == QDialog::Accepted && vselect.selectedVersion())
	{
		java = std::dynamic_pointer_cast<JavaVersion>(vselect.selectedVersion());
		ui->javaPathTextBox->setText(java->path);
	}
}

void SettingsDialog::on_javaBrowseBtn_clicked()
{
	QString dir = QFileDialog::getOpenFileName(this, tr("Find Java executable"));
	if (!dir.isNull())
	{
		ui->javaPathTextBox->setText(dir);
	}
}

void SettingsDialog::on_javaTestBtn_clicked()
{
	checker.reset(new JavaChecker());
	connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
			SLOT(checkFinished(JavaCheckResult)));
	checker->path = ui->javaPathTextBox->text();
	checker->performCheck();
}

void SettingsDialog::checkFinished(JavaCheckResult result)
{
	if (result.valid)
	{
		QString text;
		text += "Java test succeeded!\n";
		if (result.is_64bit)
			text += "Using 64bit java.\n";
		text += "\n";
		text += "Platform reported: " + result.realPlatform + "\n";
		text += "Java version reported: " + result.javaVersion;
		QMessageBox::information(this, tr("Java test success"), text);
	}
	else
	{
		QMessageBox::warning(
			this, tr("Java test failure"),
			tr("The specified java binary didn't work. You should use the auto-detect feature, "
			   "or set the path to the java executable."));
	}
}
