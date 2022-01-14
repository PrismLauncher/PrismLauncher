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

#include "settings/SettingsObject.h"
#include "tools/BaseProfiler.h"
#include "Application.h"

PastePage::PastePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PastePage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();\
    connect(ui->customAPIkeyEdit, &QLineEdit::textEdited, this, &PastePage::textEdited);
    loadSettings();
}

PastePage::~PastePage()
{
    delete ui;
}

void PastePage::loadSettings()
{
    auto s = APPLICATION->settings();
    QString keyToUse = s->get("PasteEEAPIKey").toString();
    if(keyToUse == "multimc")
    {
        ui->multimcButton->setChecked(true);
    }
    else
    {
        ui->customButton->setChecked(true);
        ui->customAPIkeyEdit->setText(keyToUse);
    }
}

void PastePage::applySettings()
{
    auto s = APPLICATION->settings();

    QString pasteKeyToUse;
    if (ui->customButton->isChecked())
        pasteKeyToUse = ui->customAPIkeyEdit->text();
    else
    {
        pasteKeyToUse =  "multimc";
    }
    s->set("PasteEEAPIKey", pasteKeyToUse);
}

bool PastePage::apply()
{
    applySettings();
    return true;
}

void PastePage::textEdited(const QString& text)
{
    ui->customButton->setChecked(true);
}
