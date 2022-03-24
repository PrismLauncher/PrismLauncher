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

#include "ProxyPage.h"
#include "ui_ProxyPage.h"

#include <QTabBar>

#include "settings/SettingsObject.h"
#include "Application.h"
#include "Application.h"

ProxyPage::ProxyPage(QWidget *parent) : QWidget(parent), ui(new Ui::ProxyPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    loadSettings();
    updateCheckboxStuff();

    connect(ui->proxyGroup, SIGNAL(buttonClicked(int)), SLOT(proxyChanged(int)));
}

ProxyPage::~ProxyPage()
{
    delete ui;
}

bool ProxyPage::apply()
{
    applySettings();
    return true;
}

void ProxyPage::updateCheckboxStuff()
{
    ui->proxyAddrBox->setEnabled(!ui->proxyNoneBtn->isChecked() &&
                                 !ui->proxyDefaultBtn->isChecked());
    ui->proxyAuthBox->setEnabled(!ui->proxyNoneBtn->isChecked() &&
                                 !ui->proxyDefaultBtn->isChecked());
}

void ProxyPage::proxyChanged(int)
{
    updateCheckboxStuff();
}

void ProxyPage::applySettings()
{
    auto s = APPLICATION->settings();

    // Proxy
    QString proxyType = "None";
    if (ui->proxyDefaultBtn->isChecked())
        proxyType = "Default";
    else if (ui->proxyNoneBtn->isChecked())
        proxyType = "None";
    else if (ui->proxySOCKS5Btn->isChecked())
        proxyType = "SOCKS5";
    else if (ui->proxyHTTPBtn->isChecked())
        proxyType = "HTTP";

    s->set("ProxyType", proxyType);
    s->set("ProxyAddr", ui->proxyAddrEdit->text());
    s->set("ProxyPort", ui->proxyPortEdit->value());
    s->set("ProxyUser", ui->proxyUserEdit->text());
    s->set("ProxyPass", ui->proxyPassEdit->text());

    APPLICATION->updateProxySettings(
        proxyType,
        ui->proxyAddrEdit->text(),
        ui->proxyPortEdit->value(),
        ui->proxyUserEdit->text(),
        ui->proxyPassEdit->text()
    );
}
void ProxyPage::loadSettings()
{
    auto s = APPLICATION->settings();
    // Proxy
    QString proxyType = s->get("ProxyType").toString();
    if (proxyType == "Default")
        ui->proxyDefaultBtn->setChecked(true);
    else if (proxyType == "None")
        ui->proxyNoneBtn->setChecked(true);
    else if (proxyType == "SOCKS5")
        ui->proxySOCKS5Btn->setChecked(true);
    else if (proxyType == "HTTP")
        ui->proxyHTTPBtn->setChecked(true);

    ui->proxyAddrEdit->setText(s->get("ProxyAddr").toString());
    ui->proxyPortEdit->setValue(s->get("ProxyPort").value<uint16_t>());
    ui->proxyUserEdit->setText(s->get("ProxyUser").toString());
    ui->proxyPassEdit->setText(s->get("ProxyPass").toString());
}

void ProxyPage::retranslate()
{
    ui->retranslateUi(this);
}
