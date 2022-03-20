// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "APIPage.h"
#include "ui_APIPage.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTabBar>
#include <QVariant>

#include "settings/SettingsObject.h"
#include "tools/BaseProfiler.h"
#include "Application.h"

APIPage::APIPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::APIPage)
{
    static QRegularExpression validUrlRegExp("https?://.+");
    ui->setupUi(this);
    ui->urlChoices->setValidator(new QRegularExpressionValidator(validUrlRegExp, ui->urlChoices));
    ui->tabWidget->tabBar()->hide();\
    loadSettings();
}

APIPage::~APIPage()
{
    delete ui;
}

void APIPage::loadSettings()
{
    auto s = APPLICATION->settings();
    QString pastebinURL = s->get("PastebinURL").toString();
    ui->urlChoices->setCurrentText(pastebinURL);
    QString msaClientID = s->get("MSAClientIDOverride").toString();
    ui->msaClientID->setText(msaClientID);
}

void APIPage::applySettings()
{
    auto s = APPLICATION->settings();
    QString pastebinURL = ui->urlChoices->currentText();
    s->set("PastebinURL", pastebinURL);
    QString msaClientID = ui->msaClientID->text();
    s->set("MSAClientIDOverride", msaClientID);
}

bool APIPage::apply()
{
    applySettings();
    return true;
}

void APIPage::retranslate()
{
    ui->retranslateUi(this);
}
