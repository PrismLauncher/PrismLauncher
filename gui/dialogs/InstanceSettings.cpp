/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Andrew Okin
 *          Peterix
 *          Orochimarufan <orochimarufan.x3@gmail.com>
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
#include "InstanceSettings.h"
#include "ui_InstanceSettings.h"
#include "gui/Platform.h"
#include "gui/dialogs/VersionSelectDialog.h"

#include "logic/JavaUtils.h"
#include "logic/NagUtils.h"
#include "logic/lists/JavaVersionList.h"
#include "logic/JavaChecker.h"

#include <QFileDialog>
#include <QMessageBox>

InstanceSettings::InstanceSettings(SettingsObject *obj, QWidget *parent)
	: QDialog(parent), ui(new Ui::InstanceSettings), m_obj(obj)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	loadSettings();
}

InstanceSettings::~InstanceSettings()
{
	delete ui;
}

void InstanceSettings::showEvent(QShowEvent *ev)
{
	QDialog::showEvent(ev);
	adjustSize();
}

void InstanceSettings::on_customCommandsGroupBox_toggled(bool state)
{
	ui->labelCustomCmdsDescription->setEnabled(state);
}

void InstanceSettings::on_buttonBox_accepted()
{
	applySettings();
	accept();
}

void InstanceSettings::on_buttonBox_rejected()
{
	reject();
}

void InstanceSettings::applySettings()
{
	// Console
	bool console = ui->consoleSettingsBox->isChecked();
	m_obj->set("OverrideConsole", console);
	if (console)
	{
		m_obj->set("ShowConsole", ui->showConsoleCheck->isChecked());
		m_obj->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());
	}
	else
	{
		m_obj->reset("ShowConsole");
		m_obj->reset("AutoCloseConsole");
	}

	// Window Size
	bool window = ui->windowSizeGroupBox->isChecked();
	m_obj->set("OverrideWindow", window);
	if (window)
	{
		m_obj->set("LaunchMaximized", ui->maximizedCheckBox->isChecked());
		m_obj->set("MinecraftWinWidth", ui->windowWidthSpinBox->value());
		m_obj->set("MinecraftWinHeight", ui->windowHeightSpinBox->value());
	}
	else
	{
		m_obj->reset("LaunchMaximized");
		m_obj->reset("MinecraftWinWidth");
		m_obj->reset("MinecraftWinHeight");
	}

	// Auto Login
	bool login = ui->accountSettingsBox->isChecked();
	m_obj->set("OverrideLogin", login);
	if (login)
	{
		m_obj->set("AutoLogin", ui->autoLoginCheckBox->isChecked());
	}
	else
	{
		m_obj->reset("AutoLogin");
	}

	// Memory
	bool memory = ui->memoryGroupBox->isChecked();
	m_obj->set("OverrideMemory", memory);
	if (memory)
	{
		m_obj->set("MinMemAlloc", ui->minMemSpinBox->value());
		m_obj->set("MaxMemAlloc", ui->maxMemSpinBox->value());
		m_obj->set("PermGen", ui->permGenSpinBox->value());
	}
	else
	{
		m_obj->reset("MinMemAlloc");
		m_obj->reset("MaxMemAlloc");
		m_obj->reset("PermGen");
	}

	// Java Settings
	bool java = ui->javaSettingsGroupBox->isChecked();
	m_obj->set("OverrideJava", java);
	if (java)
	{
		m_obj->set("JavaPath", ui->javaPathTextBox->text());
		m_obj->set("JvmArgs", ui->jvmArgsTextBox->text());

		NagUtils::checkJVMArgs(m_obj->get("JvmArgs").toString(), this->parentWidget());
	}
	else
	{
		m_obj->reset("JavaPath");
		m_obj->reset("JvmArgs");
	}

	// Custom Commands
	bool custcmd = ui->customCommandsGroupBox->isChecked();
	m_obj->set("OverrideCommands", custcmd);
	if (custcmd)
	{
		m_obj->set("PreLaunchCommand", ui->preLaunchCmdTextBox->text());
		m_obj->set("PostExitCommand", ui->postExitCmdTextBox->text());
	}
	else
	{
		m_obj->reset("PreLaunchCommand");
		m_obj->reset("PostExitCommand");
	}
}

void InstanceSettings::loadSettings()
{
	// Console
	ui->consoleSettingsBox->setChecked(m_obj->get("OverrideConsole").toBool());
	ui->showConsoleCheck->setChecked(m_obj->get("ShowConsole").toBool());
	ui->autoCloseConsoleCheck->setChecked(m_obj->get("AutoCloseConsole").toBool());

	// Window Size
	ui->windowSizeGroupBox->setChecked(m_obj->get("OverrideWindow").toBool());
	ui->maximizedCheckBox->setChecked(m_obj->get("LaunchMaximized").toBool());
	ui->windowWidthSpinBox->setValue(m_obj->get("MinecraftWinWidth").toInt());
	ui->windowHeightSpinBox->setValue(m_obj->get("MinecraftWinHeight").toInt());

	// Auto Login
	ui->accountSettingsBox->setChecked(m_obj->get("OverrideLogin").toBool());
	ui->autoLoginCheckBox->setChecked(m_obj->get("AutoLogin").toBool());

	// Memory
	ui->memoryGroupBox->setChecked(m_obj->get("OverrideMemory").toBool());
	ui->minMemSpinBox->setValue(m_obj->get("MinMemAlloc").toInt());
	ui->maxMemSpinBox->setValue(m_obj->get("MaxMemAlloc").toInt());
	ui->permGenSpinBox->setValue(m_obj->get("PermGen").toInt());

	// Java Settings
	ui->javaSettingsGroupBox->setChecked(m_obj->get("OverrideJava").toBool());
	ui->javaPathTextBox->setText(m_obj->get("JavaPath").toString());
	ui->jvmArgsTextBox->setText(m_obj->get("JvmArgs").toString());

	// Custom Commands
	ui->customCommandsGroupBox->setChecked(m_obj->get("OverrideCommands").toBool());
	ui->preLaunchCmdTextBox->setText(m_obj->get("PreLaunchCommand").toString());
	ui->postExitCmdTextBox->setText(m_obj->get("PostExitCommand").toString());
}

void InstanceSettings::on_javaDetectBtn_clicked()
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

void InstanceSettings::on_javaBrowseBtn_clicked()
{
	QString dir = QFileDialog::getOpenFileName(this, tr("Find Java executable"));
	if (!dir.isNull())
	{
		ui->javaPathTextBox->setText(dir);
	}
}

void InstanceSettings::on_javaTestBtn_clicked()
{
	checker.reset(new JavaChecker());
	connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
			SLOT(checkFinished(JavaCheckResult)));
	checker->path = ui->javaPathTextBox->text();
	checker->performCheck();
}

void InstanceSettings::checkFinished(JavaCheckResult result)
{
	if (result.valid)
	{
		QString text;
		text += "Java test succeeded!\n";
		if (result.is_64bit)
			text += "Using 64bit java.\n";
		text += "\n";
		text += "Platform reported: " + result.realPlatform;
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