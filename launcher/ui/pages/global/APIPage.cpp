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
