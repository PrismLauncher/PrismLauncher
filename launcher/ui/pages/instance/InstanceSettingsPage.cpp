// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "InstanceSettingsPage.h"
#include "ui_InstanceSettingsPage.h"

#include <QFileDialog>
#include <QDialog>
#include <QMessageBox>

#include <sys.h>

#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/widgets/CustomCommands.h"

#include "JavaCommon.h"
#include "Application.h"

#include "java/JavaInstallList.h"
#include "java/JavaUtils.h"
#include "FileSystem.h"


InstanceSettingsPage::InstanceSettingsPage(BaseInstance *inst, QWidget *parent)
    : QWidget(parent), ui(new Ui::InstanceSettingsPage), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance(inst)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings = inst->settings();
    ui->setupUi(this);

    connect(ui->openGlobalJavaSettingsButton, &QCommandLinkButton::clicked, this, &InstanceSettingsPage::globalSettingsButtonClicked);
    connect(APPLICATION, &Application::globalSettingsAboutToOpen, this, &InstanceSettingsPage::applySettings);
    connect(APPLICATION, &Application::globalSettingsClosed, this, &InstanceSettingsPage::loadSettings);
    loadSettings();
    updateThresholds();
}

bool InstanceSettingsPage::shouldDisplay() const
{
    return !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->isRunning();
}

InstanceSettingsPage::~InstanceSettingsPage()
{
    delete ui;
}

void InstanceSettingsPage::globalSettingsButtonClicked(bool)
{
    switch(ui->settingsTabs->currentIndex()) {
        case 0:
            APPLICATION->ShowGlobalSettings(this, "java-settings");
            return;
        case 1:
            APPLICATION->ShowGlobalSettings(this, "minecraft-settings");
            return;
        case 2:
            APPLICATION->ShowGlobalSettings(this, "custom-commands");
            return;
    }
}

bool InstanceSettingsPage::apply()
{
    applySettings();
    return true;
}

void InstanceSettingsPage::applySettings()
{
    SettingsObject::Lock lock(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings);

    // Miscellaneous
    bool miscellaneous = ui->miscellaneousSettingsBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverrideMiscellaneous", miscellaneous);
    if (miscellaneous)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("CloseAfterLaunch", ui->closeAfterLaunchCheck->isChecked());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("QuitAfterGameStop", ui->quitAfterGameStopCheck->isChecked());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("CloseAfterLaunch");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("QuitAfterGameStop");
    }

    // Console
    bool console = ui->consoleSettingsBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverrideConsole", console);
    if (console)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("ShowConsole", ui->showConsoleCheck->isChecked());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("ShowConsoleOnError", ui->showConsoleErrorCheck->isChecked());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("ShowConsole");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("AutoCloseConsole");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("ShowConsoleOnError");
    }

    // Window Size
    bool window = ui->windowSizeGroupBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverrideWindow", window);
    if (window)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("LaunchMaximized", ui->maximizedCheckBox->isChecked());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("MinecraftWinWidth", ui->windowWidthSpinBox->value());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("MinecraftWinHeight", ui->windowHeightSpinBox->value());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("LaunchMaximized");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("MinecraftWinWidth");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("MinecraftWinHeight");
    }

    // Memory
    bool memory = ui->memoryGroupBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverrideMemory", memory);
    if (memory)
    {
        int min = ui->minMemSpinBox->value();
        int max = ui->maxMemSpinBox->value();
        if(min < max)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("MinMemAlloc", min);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("MaxMemAlloc", max);
        }
        else
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("MinMemAlloc", max);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("MaxMemAlloc", min);
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("PermGen", ui->permGenSpinBox->value());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("MinMemAlloc");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("MaxMemAlloc");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("PermGen");
    }

    // Java Install Settings
    bool javaInstall = ui->javaSettingsGroupBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverrideJavaLocation", javaInstall);
    if (javaInstall)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("JavaPath", ui->javaPathTextBox->text());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("IgnoreJavaCompatibility", ui->skipCompatibilityCheckbox->isChecked());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("JavaPath");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("IgnoreJavaCompatibility");
    }

    // Java arguments
    bool javaArgs = ui->javaArgumentsGroupBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverrideJavaArgs", javaArgs);
    if(javaArgs)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("JvmArgs", ui->jvmArgsTextBox->toPlainText().replace("\n", " "));
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("JvmArgs");
    }

    // old generic 'override both' is removed.
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("OverrideJava");

    // Custom Commands
    bool custcmd = ui->customCommands->checked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverrideCommands", custcmd);
    if (custcmd)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("PreLaunchCommand", ui->customCommands->prelaunchCommand());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("WrapperCommand", ui->customCommands->wrapperCommand());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("PostExitCommand", ui->customCommands->postexitCommand());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("PreLaunchCommand");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("WrapperCommand");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("PostExitCommand");
    }

    // Workarounds
    bool workarounds = ui->nativeWorkaroundsGroupBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverrideNativeWorkarounds", workarounds);
    if(workarounds)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("UseNativeOpenAL", ui->useNativeOpenALCheck->isChecked());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("UseNativeGLFW", ui->useNativeGLFWCheck->isChecked());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("UseNativeOpenAL");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("UseNativeGLFW");
    }

    // Performance
    bool performance = ui->perfomanceGroupBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverridePerformance", performance);
    if(performance)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("EnableFeralGamemode", ui->enableFeralGamemodeCheck->isChecked());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("EnableMangoHud", ui->enableMangoHud->isChecked());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("UseDiscreteGpu", ui->useDiscreteGpuCheck->isChecked());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("EnableFeralGamemode");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("EnableMangoHud");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("UseDiscreteGpu");
    }

    // Game time
    bool gameTime = ui->gameTimeGroupBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("OverrideGameTime", gameTime);
    if (gameTime)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("ShowGameTime", ui->showGameTime->isChecked());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("RecordGameTime", ui->recordGameTime->isChecked());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("ShowGameTime");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("RecordGameTime");
    }

    // Join server on launch
    bool joinServerOnLaunch = ui->serverJoinGroupBox->isChecked();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("JoinServerOnLaunch", joinServerOnLaunch);
    if (joinServerOnLaunch)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("JoinServerOnLaunchAddress", ui->serverJoinAddress->text());
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->reset("JoinServerOnLaunchAddress");
    }

    // FIXME: This should probably be called by a signal instead
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->updateRuntimeContext();
}

void InstanceSettingsPage::loadSettings()
{
    // Miscellaneous
    ui->miscellaneousSettingsBox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideMiscellaneous").toBool());
    ui->closeAfterLaunchCheck->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("CloseAfterLaunch").toBool());
    ui->quitAfterGameStopCheck->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("QuitAfterGameStop").toBool());

    // Console
    ui->consoleSettingsBox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideConsole").toBool());
    ui->showConsoleCheck->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("ShowConsole").toBool());
    ui->autoCloseConsoleCheck->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("AutoCloseConsole").toBool());
    ui->showConsoleErrorCheck->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("ShowConsoleOnError").toBool());

    // Window Size
    ui->windowSizeGroupBox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideWindow").toBool());
    ui->maximizedCheckBox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("LaunchMaximized").toBool());
    ui->windowWidthSpinBox->setValue(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("MinecraftWinWidth").toInt());
    ui->windowHeightSpinBox->setValue(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("MinecraftWinHeight").toInt());

    // Memory
    ui->memoryGroupBox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideMemory").toBool());
    int min = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("MinMemAlloc").toInt();
    int max = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("MaxMemAlloc").toInt();
    if(min < max)
    {
        ui->minMemSpinBox->setValue(min);
        ui->maxMemSpinBox->setValue(max);
    }
    else
    {
        ui->minMemSpinBox->setValue(max);
        ui->maxMemSpinBox->setValue(min);
    }
    ui->permGenSpinBox->setValue(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("PermGen").toInt());
    bool permGenVisible = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("PermGenVisible").toBool();
    ui->permGenSpinBox->setVisible(permGenVisible);
    ui->labelPermGen->setVisible(permGenVisible);
    ui->labelPermgenNote->setVisible(permGenVisible);


    // Java Settings
    bool overrideJava = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideJava").toBool();
    bool overrideLocation = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideJavaLocation").toBool() || overrideJava;
    bool overrideArgs = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideJavaArgs").toBool() || overrideJava;

    ui->javaSettingsGroupBox->setChecked(overrideLocation);
    ui->javaPathTextBox->setText(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("JavaPath").toString());
    ui->skipCompatibilityCheckbox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("IgnoreJavaCompatibility").toBool());

    ui->javaArgumentsGroupBox->setChecked(overrideArgs);
    ui->jvmArgsTextBox->setPlainText(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("JvmArgs").toString());

    // Custom commands
    ui->customCommands->initialize(
        true,
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideCommands").toBool(),
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("PreLaunchCommand").toString(),
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("WrapperCommand").toString(),
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("PostExitCommand").toString()
    );

    // Workarounds
    ui->nativeWorkaroundsGroupBox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideNativeWorkarounds").toBool());
    ui->useNativeGLFWCheck->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("UseNativeGLFW").toBool());
    ui->useNativeOpenALCheck->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("UseNativeOpenAL").toBool());

    // Performance
    ui->perfomanceGroupBox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverridePerformance").toBool());
    ui->enableFeralGamemodeCheck->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("EnableFeralGamemode").toBool());
    ui->enableMangoHud->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("EnableMangoHud").toBool());
    ui->useDiscreteGpuCheck->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("UseDiscreteGpu").toBool());

#if !defined(Q_OS_LINUX)
    ui->settingsTabs->setTabVisible(ui->settingsTabs->indexOf(ui->performancePage), false);
#endif

    if (!(APPLICATION->capabilities() & Application::SupportsGameMode)) {
        ui->enableFeralGamemodeCheck->setDisabled(true);
        ui->enableFeralGamemodeCheck->setToolTip(tr("Feral Interactive's GameMode could not be found on your system."));
    }

    if (!(APPLICATION->capabilities() & Application::SupportsMangoHud)) {
        ui->enableMangoHud->setDisabled(true);
        ui->enableMangoHud->setToolTip(tr("MangoHud could not be found on your system."));
    }

    // Miscellanous
    ui->gameTimeGroupBox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideGameTime").toBool());
    ui->showGameTime->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("ShowGameTime").toBool());
    ui->recordGameTime->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("RecordGameTime").toBool());

    ui->serverJoinGroupBox->setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("JoinServerOnLaunch").toBool());
    ui->serverJoinAddress->setText(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("JoinServerOnLaunchAddress").toString());
}

void InstanceSettingsPage::on_javaDetectBtn_clicked()
{
    if (JavaUtils::getJavaCheckPath().isEmpty()) {
        JavaCommon::javaCheckNotFound(this);
        return;
    }

    JavaInstallPtr java;

    VersionSelectDialog vselect(APPLICATION->javalist().get(), tr("Select a Java version"), this, true);
    vselect.setResizeOn(2);
    vselect.exec();

    if (vselect.result() == QDialog::Accepted && vselect.selectedVersion())
    {
        java = std::dynamic_pointer_cast<JavaInstall>(vselect.selectedVersion());
        ui->javaPathTextBox->setText(java->path);
        bool visible = java->id.requiresPermGen() && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->get("OverrideMemory").toBool();
        ui->permGenSpinBox->setVisible(visible);
        ui->labelPermGen->setVisible(visible);
        ui->labelPermgenNote->setVisible(visible);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("PermGenVisible", visible);
    }
}

void InstanceSettingsPage::on_javaBrowseBtn_clicked()
{
    QString raw_path = QFileDialog::getOpenFileName(this, tr("Find Java executable"));

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if(raw_path.isEmpty())
    {
        return;
    }
    QString cooked_path = FS::NormalizePath(raw_path);

    QFileInfo javaInfo(cooked_path);
    if(!javaInfo.exists() || !javaInfo.isExecutable())
    {
        return;
    }
    ui->javaPathTextBox->setText(cooked_path);

    // custom Java could be anything... enable perm gen option
    ui->permGenSpinBox->setVisible(true);
    ui->labelPermGen->setVisible(true);
    ui->labelPermgenNote->setVisible(true);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings->set("PermGenVisible", true);
}

void InstanceSettingsPage::on_javaTestBtn_clicked()
{
    if(checker)
    {
        return;
    }
    checker.reset(new JavaCommon::TestCheck(
        this, ui->javaPathTextBox->text(), ui->jvmArgsTextBox->toPlainText().replace("\n", " "),
        ui->minMemSpinBox->value(), ui->maxMemSpinBox->value(), ui->permGenSpinBox->value()));
    connect(checker.get(), SIGNAL(finished()), SLOT(checkerFinished()));
    checker->run();
}

void InstanceSettingsPage::on_maxMemSpinBox_valueChanged(int i)
{
    updateThresholds();
}

void InstanceSettingsPage::checkerFinished()
{
    checker.reset();
}

void InstanceSettingsPage::retranslate()
{
    ui->retranslateUi(this);
    ui->customCommands->retranslate();  // TODO: why is this seperate from the others?
}

void InstanceSettingsPage::updateThresholds()
{
    auto sysMiB = Sys::getSystemRam() / Sys::mebibyte;
    unsigned int maxMem = ui->maxMemSpinBox->value();

    QString iconName;

    if (maxMem >= sysMiB) {
        iconName = "status-bad";
        ui->labelMaxMemIcon->setToolTip(tr("Your maximum memory allocation exceeds your system memory capacity."));
    } else if (maxMem > (sysMiB * 0.9)) {
        iconName = "status-yellow";
        ui->labelMaxMemIcon->setToolTip(tr("Your maximum memory allocation approaches your system memory capacity."));
    } else {
        iconName = "status-good";
        ui->labelMaxMemIcon->setToolTip("");
    }

    {
        auto height = ui->labelMaxMemIcon->fontInfo().pixelSize();
        QIcon icon = APPLICATION->getThemedIcon(iconName);
        QPixmap pix = icon.pixmap(height, height);
        ui->labelMaxMemIcon->setPixmap(pix);
    }
}
