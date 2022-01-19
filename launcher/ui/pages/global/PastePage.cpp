/* Copyright 2013-2021 MultiMC & PolyMC Contributors
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

#include "PastePage.h"
#include "ui_PastePage.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTabBar>
#include <QVariant>

#include "settings/SettingsObject.h"
#include "tools/BaseProfiler.h"
#include "Application.h"

PastePage::PastePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PastePage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();\
    loadSettings();
}

PastePage::~PastePage()
{
    delete ui;
}

void PastePage::loadSettings()
{
    auto s = APPLICATION->settings();
    QString pastebinURL = s->get("PastebinURL").toString();
    ui->urlChoices->setCurrentText(pastebinURL);
}

void PastePage::applySettings()
{
    auto s = APPLICATION->settings();
    QString pastebinURL = ui->urlChoices->currentText();
    s->set("PastebinURL", pastebinURL);
}

bool PastePage::apply()
{
    applySettings();
    return true;
}
