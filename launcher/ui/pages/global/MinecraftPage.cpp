// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "MinecraftPage.h"
#include "ui_MinecraftPage.h"

#include <QMessageBox>
#include <QDir>
#include <QTabBar>

#include "settings/SettingsObject.h"
#include "Application.h"

MinecraftPage::MinecraftPage(QWidget *parent) : QWidget(parent), ui(new Ui::MinecraftPage)
{
    ui->setupUi(this);
    loadSettings();
    updateCheckboxStuff();
}

MinecraftPage::~MinecraftPage()
{
    delete ui;
}

bool MinecraftPage::apply()
{
    applySettings();
    return true;
}

void MinecraftPage::updateCheckboxStuff()
{
    ui->windowWidthSpinBox->setEnabled(!ui->maximizedCheckBox->isChecked());
    ui->windowHeightSpinBox->setEnabled(!ui->maximizedCheckBox->isChecked());
}

void MinecraftPage::on_maximizedCheckBox_clicked(bool checked)
{
    Q_UNUSED(checked);
    updateCheckboxStuff();
}

void MinecraftPage::applySettings()
{
    auto s = APPLICATION->settings();

    // Window Size
    s->set("LaunchMaximized", ui->maximizedCheckBox->isChecked());
    s->set("MinecraftWinWidth", ui->windowWidthSpinBox->value());
    s->set("MinecraftWinHeight", ui->windowHeightSpinBox->value());

    // Native library workarounds
    s->set("UseNativeOpenAL", ui->useNativeOpenALCheck->isChecked());
    s->set("UseNativeGLFW", ui->useNativeGLFWCheck->isChecked());

    // Peformance related options
    s->set("EnableFeralGamemode", ui->enableFeralGamemodeCheck->isChecked());
    s->set("EnableMangoHud", ui->enableMangoHud->isChecked());
    s->set("UseDiscreteGpu", ui->useDiscreteGpuCheck->isChecked());

    // Game time
    s->set("ShowGameTime", ui->showGameTime->isChecked());
    s->set("ShowGlobalGameTime", ui->showGlobalGameTime->isChecked());
    s->set("RecordGameTime", ui->recordGameTime->isChecked());

    // Miscellaneous
    s->set("CloseAfterLaunch", ui->closeAfterLaunchCheck->isChecked());
    s->set("QuitAfterGameStop", ui->quitAfterGameStopCheck->isChecked());
}

void MinecraftPage::loadSettings()
{
    auto s = APPLICATION->settings();

    // Window Size
    ui->maximizedCheckBox->setChecked(s->get("LaunchMaximized").toBool());
    ui->windowWidthSpinBox->setValue(s->get("MinecraftWinWidth").toInt());
    ui->windowHeightSpinBox->setValue(s->get("MinecraftWinHeight").toInt());

    ui->useNativeOpenALCheck->setChecked(s->get("UseNativeOpenAL").toBool());
    ui->useNativeGLFWCheck->setChecked(s->get("UseNativeGLFW").toBool());

    ui->enableFeralGamemodeCheck->setChecked(s->get("EnableFeralGamemode").toBool());
    ui->enableMangoHud->setChecked(s->get("EnableMangoHud").toBool());
    ui->useDiscreteGpuCheck->setChecked(s->get("UseDiscreteGpu").toBool());

#if !defined(Q_OS_LINUX)
    ui->perfomanceGroupBox->setVisible(false);
#endif

    if (!(APPLICATION->capabilities() & Application::SupportsGameMode)) {
        ui->enableFeralGamemodeCheck->setDisabled(true);
        ui->enableFeralGamemodeCheck->setToolTip(tr("Feral Interactive's GameMode could not be found on your system."));
    }

    if (!(APPLICATION->capabilities() & Application::SupportsMangoHud)) {
        ui->enableMangoHud->setDisabled(true);
        ui->enableMangoHud->setToolTip(tr("MangoHud could not be found on your system."));
    }

    ui->showGameTime->setChecked(s->get("ShowGameTime").toBool());
    ui->showGlobalGameTime->setChecked(s->get("ShowGlobalGameTime").toBool());
    ui->recordGameTime->setChecked(s->get("RecordGameTime").toBool());

    ui->closeAfterLaunchCheck->setChecked(s->get("CloseAfterLaunch").toBool());
    ui->quitAfterGameStopCheck->setChecked(s->get("QuitAfterGameStop").toBool());
}

void MinecraftPage::retranslate()
{
    ui->retranslateUi(this);
}
