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

#include "NewComponentDialog.h"
#include "Application.h"
#include "ui_NewComponentDialog.h"

#include <BaseVersion.h>
#include <InstanceList.h>
#include <icons/IconList.h>
#include <tasks/Task.h>

#include "IconPickerDialog.h"
#include "ProgressDialog.h"
#include "VersionSelectDialog.h"

#include <QFileDialog>
#include <QLayout>
#include <QPushButton>
#include <QValidator>

#include <meta/Index.h>
#include <meta/VersionList.h>

NewComponentDialog::NewComponentDialog(const QString& initialName, const QString& initialUid, QWidget* parent)
    : QDialog(parent), ui(new Ui::NewComponentDialog)
{
    ui->setupUi(this);
    resize(minimumSizeHint());

    ui->nameTextBox->setText(initialName);
    ui->uidTextBox->setText(initialUid);

    connect(ui->nameTextBox, &QLineEdit::textChanged, this, &NewComponentDialog::updateDialogState);
    connect(ui->uidTextBox, &QLineEdit::textChanged, this, &NewComponentDialog::updateDialogState);

    ui->nameTextBox->setFocus();

    originalPlaceholderText = ui->uidTextBox->placeholderText();
    updateDialogState();
}

NewComponentDialog::~NewComponentDialog()
{
    delete ui;
}

void NewComponentDialog::updateDialogState()
{
    auto protoUid = ui->nameTextBox->text().toLower();
    protoUid.remove(QRegularExpression("[^a-z]"));
    if (protoUid.isEmpty()) {
        ui->uidTextBox->setPlaceholderText(originalPlaceholderText);
    } else {
        QString suggestedUid = "org.multimc.custom." + protoUid;
        ui->uidTextBox->setPlaceholderText(suggestedUid);
    }
    bool allowOK = !name().isEmpty() && !uid().isEmpty() && !uidBlacklist.contains(uid());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allowOK);
}

QString NewComponentDialog::name() const
{
    auto result = ui->nameTextBox->text();
    if (result.size()) {
        return result.trimmed();
    }
    return QString();
}

QString NewComponentDialog::uid() const
{
    auto result = ui->uidTextBox->text();
    if (result.size()) {
        return result.trimmed();
    }
    result = ui->uidTextBox->placeholderText();
    if (result.size() && result != originalPlaceholderText) {
        return result.trimmed();
    }
    return QString();
}

void NewComponentDialog::setBlacklist(QStringList badUids)
{
    uidBlacklist = badUids;
}
