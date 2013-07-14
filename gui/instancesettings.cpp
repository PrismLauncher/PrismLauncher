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

#include "instancesettings.h"
#include "ui_instancesettings.h"

InstanceSettings::InstanceSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InstanceSettings)
{
    ui->setupUi(this);
}

InstanceSettings::~InstanceSettings()
{
    delete ui;
}

void InstanceSettings::on_customCommandsGroupBox_toggled(bool state)
{
    ui->labelCustomCmdsDescription->setEnabled(state);
}


void InstanceSettings::applySettings(SettingsObject *s)
{

    // Console
    s->set("ShowConsole", ui->showConsoleCheck->isChecked());
    s->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());
    s->set("OverrideConsole", ui->consoleSettingsBox->isChecked());

    // Window Size
    s->set("LaunchCompatMode", ui->compatModeCheckBox->isChecked());
    s->set("LaunchMaximized", ui->maximizedCheckBox->isChecked());
    s->set("MinecraftWinWidth", ui->windowWidthSpinBox->value());
    s->set("MinecraftWinHeight", ui->windowHeightSpinBox->value());
    s->set("OverrideWindow", ui->windowSizeGroupBox->isChecked());

    // Auto Login
    s->set("AutoLogin", ui->autoLoginChecBox->isChecked());
    s->set("OverrideLogin", ui->accountSettingsGroupBox->isChecked());

    // Memory
    s->set("MinMemAlloc", ui->minMemSpinBox->value());
    s->set("MaxMemAlloc", ui->maxMemSpinBox->value());
    s->set("OverrideMemory", ui->memoryGroupBox->isChecked());

    // Java Settings
    s->set("JavaPath", ui->javaPathTextBox->text());
    s->set("JvmArgs", ui->jvmArgsTextBox->text());
    s->set("OverrideJava", ui->javaSettingsGroupBox->isChecked());

    // Custom Commands
    s->set("PreLaunchCommand", ui->preLaunchCmdTextBox->text());
    s->set("PostExitCommand", ui->postExitCmdTextBox->text());
    s->set("OverrideCommands", ui->customCommandsGroupBox->isChecked());
}

void InstanceSettings::loadSettings(SettingsObject *s)
{

    // Console
    ui->showConsoleCheck->setChecked(s->get("ShowConsole").toBool());
    ui->autoCloseConsoleCheck->setChecked(s->get("AutoCloseConsole").toBool());
    ui->consoleSettingsBox->setChecked(s->get("OverrideConsole").toBool());

    // Window Size
    ui->compatModeCheckBox->setChecked(s->get("LaunchCompatMode").toBool());
    ui->maximizedCheckBox->setChecked(s->get("LaunchMaximized").toBool());
    ui->windowWidthSpinBox->setValue(s->get("MinecraftWinWidth").toInt());
    ui->windowHeightSpinBox->setValue(s->get("MinecraftWinHeight").toInt());
    ui->windowSizeGroupBox->setChecked(s->get("OverrideWindow").toBool());

    // Auto Login
    ui->autoLoginChecBox->setChecked(s->get("AutoLogin").toBool());
    ui->accountSettingsGroupBox->setChecked(s->get("OverrideLogin").toBool());

    // Memory
    ui->minMemSpinBox->setValue(s->get("MinMemAlloc").toInt());
    ui->maxMemSpinBox->setValue(s->get("MaxMemAlloc").toInt());
    ui->memoryGroupBox->setChecked(s->get("OverrideMemory").toBool());

    // Java Settings
    ui->javaPathTextBox->setText(s->get("JavaPath").toString());
    ui->jvmArgsTextBox->setText(s->get("JvmArgs").toString());
    ui->javaSettingsGroupBox->setChecked(s->get("OverrideJava").toBool());

    // Custom Commands
    ui->preLaunchCmdTextBox->setText(s->get("PreLaunchCommand").toString());
    ui->postExitCmdTextBox->setText(s->get("PostExitCommand").toString());
    ui->customCommandsGroupBox->setChecked(s->get("OverrideCommands").toBool());
}
