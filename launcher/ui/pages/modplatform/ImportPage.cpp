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

#include "ImportPage.h"
#include "ui_ImportPage.h"

#include <QFileDialog>
#include <QValidator>

#include "ui/dialogs/NewInstanceDialog.h"

#include "InstanceImportTask.h"


class UrlValidator : public QValidator
{
public:
    using QValidator::QValidator;

    State validate(QString &in, int &pos) const
    {
        const QUrl url(in);
        if (url.isValid() && !url.isRelative() && !url.isEmpty())
        {
            return Acceptable;
        }
        else if (QFile::exists(in))
        {
            return Acceptable;
        }
        else
        {
            return Intermediate;
        }
    }
};

ImportPage::ImportPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::ImportPage), dialog(dialog)
{
    ui->setupUi(this);
    ui->modpackEdit->setValidator(new UrlValidator(ui->modpackEdit));
    connect(ui->modpackEdit, &QLineEdit::textChanged, this, &ImportPage::updateState);
}

ImportPage::~ImportPage()
{
    delete ui;
}

bool ImportPage::shouldDisplay() const
{
    return true;
}

void ImportPage::retranslate()
{
    ui->retranslateUi(this);
}

void ImportPage::openedImpl()
{
    updateState();
}

void ImportPage::updateState()
{
    if(!isOpened)
    {
        return;
    }
    if(ui->modpackEdit->hasAcceptableInput())
    {
        QString input = ui->modpackEdit->text();
        auto url = QUrl::fromUserInput(input);
        if(url.isLocalFile())
        {
            // FIXME: actually do some validation of what's inside here... this is fake AF
            QFileInfo fi(input);
            // mrpack is a modrinth pack
            if(fi.exists() && (fi.suffix() == "zip" || fi.suffix() == "mrpack"))
            {
                QFileInfo fi(url.fileName());
                dialog->setSuggestedPack(fi.completeBaseName(), new InstanceImportTask(url));
                dialog->setSuggestedIcon("default");
            }
        }
        else
        {
            if(input.endsWith("?client=y")) {
                input.chop(9);
                input.append("/file");
                url = QUrl::fromUserInput(input);
            }
            // hook, line and sinker.
            QFileInfo fi(url.fileName());
            dialog->setSuggestedPack(fi.completeBaseName(), new InstanceImportTask(url));
            dialog->setSuggestedIcon("default");
        }
    }
    else
    {
        dialog->setSuggestedPack();
    }
}

void ImportPage::setUrl(const QString& url)
{
    ui->modpackEdit->setText(url);
    updateState();
}

void ImportPage::on_modpackBtn_clicked()
{
    auto filter = QMimeDatabase().mimeTypeForName("application/zip").filterString();
    filter += ";;" + tr("Modrinth pack (*.mrpack)");
    const QUrl url = QFileDialog::getOpenFileUrl(this, tr("Choose modpack"), modpackUrl(), filter);
    if (url.isValid())
    {
        if (url.isLocalFile())
        {
            ui->modpackEdit->setText(url.toLocalFile());
        }
        else
        {
            ui->modpackEdit->setText(url.toString());
        }
    }
}


QUrl ImportPage::modpackUrl() const
{
    const QUrl url(ui->modpackEdit->text());
    if (url.isValid() && !url.isRelative() && !url.host().isEmpty())
    {
        return url;
    }
    else
    {
        return QUrl::fromLocalFile(ui->modpackEdit->text());
    }
}
