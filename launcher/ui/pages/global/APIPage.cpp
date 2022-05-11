// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (c) 2022 Lenny McLennington <lenny@sneed.church>
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
#include "net/PasteUpload.h"

APIPage::APIPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::APIPage)
{
    // this is here so you can reorder the entries in the combobox without messing stuff up
    unsigned int comboBoxEntries[] = {
        PasteUpload::PasteType::Mclogs,
        PasteUpload::PasteType::NullPointer,
        PasteUpload::PasteType::PasteGG,
        PasteUpload::PasteType::Hastebin
    };

    static QRegularExpression validUrlRegExp("https?://.+");

    ui->setupUi(this);

    for (auto pasteType : comboBoxEntries) {
        ui->pasteTypeComboBox->addItem(PasteUpload::PasteTypes.at(pasteType).name, pasteType);
    }

    connect(ui->pasteTypeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &APIPage::updateBaseURLPlaceholder);
    // This function needs to be called even when the ComboBox's index is still in its default state.
    updateBaseURLPlaceholder(ui->pasteTypeComboBox->currentIndex());
    ui->baseURLEntry->setValidator(new QRegularExpressionValidator(validUrlRegExp, ui->baseURLEntry));
    ui->tabWidget->tabBar()->hide();

    loadSettings();
}

APIPage::~APIPage()
{
    delete ui;
}

void APIPage::updateBaseURLPlaceholder(int index)
{
    ui->baseURLEntry->setPlaceholderText(PasteUpload::PasteTypes.at(ui->pasteTypeComboBox->itemData(index).toUInt()).defaultBase);
}

void APIPage::loadSettings()
{
    auto s = APPLICATION->settings();

    unsigned int pasteType = s->get("PastebinType").toUInt();
    QString pastebinURL = s->get("PastebinCustomAPIBase").toString();

    ui->baseURLEntry->setText(pastebinURL);
    int pasteTypeIndex = ui->pasteTypeComboBox->findData(pasteType);
    if (pasteTypeIndex == -1)
    {
        pasteTypeIndex = ui->pasteTypeComboBox->findData(PasteUpload::PasteType::Mclogs);
        ui->baseURLEntry->clear();
    }

    ui->pasteTypeComboBox->setCurrentIndex(pasteTypeIndex);

    QString msaClientID = s->get("MSAClientIDOverride").toString();
    ui->msaClientID->setText(msaClientID);
    QString curseKey = s->get("CFKeyOverride").toString();
    ui->curseKey->setText(curseKey);
}

void APIPage::applySettings()
{
    auto s = APPLICATION->settings();

    s->set("PastebinType", ui->pasteTypeComboBox->currentData().toUInt());
    s->set("PastebinCustomAPIBase", ui->baseURLEntry->text());

    QString msaClientID = ui->msaClientID->text();
    s->set("MSAClientIDOverride", msaClientID);
    QString curseKey = ui->curseKey->text();
    s->set("CFKeyOverride", curseKey);
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
