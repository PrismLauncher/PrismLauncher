// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (c) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "CustomCommandsPage.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QTabBar>

CustomCommandsPage::CustomCommandsPage(QWidget* parent): QWidget(parent)
{

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    auto tabWidget = new QTabWidget(this);
    tabWidget->setObjectName(QStringLiteral("tabWidget"));
    commands = new CustomCommands(this);
    commands->setContentsMargins(6, 6, 6, 6);
    tabWidget->addTab(commands, "Foo");
    tabWidget->tabBar()->hide();
    verticalLayout->addWidget(tabWidget);
    loadSettings();
}

CustomCommandsPage::~CustomCommandsPage()
{
}

bool CustomCommandsPage::apply()
{
    applySettings();
    return true;
}

void CustomCommandsPage::applySettings()
{
    auto s = APPLICATION->settings();
    s->set("PreLaunchCommand", commands->prelaunchCommand());
    s->set("WrapperCommand", commands->wrapperCommand());
    s->set("PostExitCommand", commands->postexitCommand());
}

void CustomCommandsPage::loadSettings()
{
    auto s = APPLICATION->settings();
    commands->initialize(
        false,
        true,
        s->get("PreLaunchCommand").toString(),
        s->get("WrapperCommand").toString(),
        s->get("PostExitCommand").toString()
    );
}

void CustomCommandsPage::retranslate()
{
    commands->retranslate();
}
