// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "CustomCommands.h"
#include "ui_CustomCommands.h"

CustomCommands::~CustomCommands()
{
    delete ui;
}

CustomCommands::CustomCommands(QWidget* parent) : QWidget(parent), ui(new Ui::CustomCommands)
{
    ui->setupUi(this);
}

void CustomCommands::initialize(bool checkable, bool checked, const QString& prelaunch, const QString& wrapper, const QString& postexit)
{
    ui->customCommandsGroupBox->setCheckable(checkable);
    if (checkable) {
        ui->customCommandsGroupBox->setChecked(checked);
    }
    ui->preLaunchCmdTextBox->setText(prelaunch);
    ui->wrapperCmdTextBox->setText(wrapper);
    ui->postExitCmdTextBox->setText(postexit);
}

void CustomCommands::retranslate()
{
    ui->retranslateUi(this);
}

bool CustomCommands::checked() const
{
    if (!ui->customCommandsGroupBox->isCheckable())
        return true;
    return ui->customCommandsGroupBox->isChecked();
}

QString CustomCommands::prelaunchCommand() const
{
    return ui->preLaunchCmdTextBox->text();
}

QString CustomCommands::wrapperCommand() const
{
    return ui->wrapperCmdTextBox->text();
}

QString CustomCommands::postexitCommand() const
{
    return ui->postExitCmdTextBox->text();
}
