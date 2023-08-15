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

#include <QFileDialog>
#include <QFileInfo>
#include <QPainter>

#include <FileSystem.h>

#include <minecraft/services/CapeChange.h>
#include <minecraft/services/SkinUpload.h>
#include <tasks/SequentialTask.h>

#include "CustomMessageBox.h"
#include "ProgressDialog.h"
#include "SkinUploadDialog.h"
#include "ui_SkinUploadDialog.h"

void SkinUploadDialog::on_buttonBox_rejected()
{
    close();
}

void SkinUploadDialog::on_buttonBox_accepted()
{
    QString fileName;
    QString input = ui->skinPathTextBox->text();
    ProgressDialog prog(this);
    SequentialTask skinUpload;

    if (!input.isEmpty()) {
        QRegularExpression urlPrefixMatcher(QRegularExpression::anchoredPattern("^([a-z]+)://.+$"));
        bool isLocalFile = false;
        // it has an URL prefix -> it is an URL
        if (urlPrefixMatcher.match(input).hasMatch()) {
            QUrl fileURL = input;
            if (fileURL.isValid()) {
                // local?
                if (fileURL.isLocalFile()) {
                    isLocalFile = true;
                    fileName = fileURL.toLocalFile();
                } else {
                    CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Using remote URLs for setting skins is not implemented yet."),
                                                 QMessageBox::Warning)
                        ->exec();
                    close();
                    return;
                }
            } else {
                CustomMessageBox::selectable(this, tr("Skin Upload"), tr("You cannot use an invalid URL for uploading skins."),
                                             QMessageBox::Warning)
                    ->exec();
                close();
                return;
            }
        } else {
            // just assume it's a path then
            isLocalFile = true;
            fileName = ui->skinPathTextBox->text();
        }
        if (isLocalFile && !QFile::exists(fileName)) {
            CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Skin file does not exist!"), QMessageBox::Warning)->exec();
            close();
            return;
        }
        SkinUpload::Model model = SkinUpload::STEVE;
        if (ui->steveBtn->isChecked()) {
            model = SkinUpload::STEVE;
        } else if (ui->alexBtn->isChecked()) {
            model = SkinUpload::ALEX;
        }
        skinUpload.addTask(shared_qobject_ptr<SkinUpload>(new SkinUpload(this, m_acct->accessToken(), FS::read(fileName), model)));
    }

    auto selectedCape = ui->capeCombo->currentData().toString();
    if (selectedCape != m_acct->accountData()->minecraftProfile.currentCape) {
        skinUpload.addTask(shared_qobject_ptr<CapeChange>(new CapeChange(this, m_acct->accessToken(), selectedCape)));
    }
    if (prog.execWithTask(&skinUpload) != QDialog::Accepted) {
        CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Failed to upload skin!"), QMessageBox::Warning)->exec();
        close();
        return;
    }
    CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Success"), QMessageBox::Information)->exec();
    close();
}

void SkinUploadDialog::on_skinBrowseBtn_clicked()
{
    auto filter = QMimeDatabase().mimeTypeForName("image/png").filterString();
    QString raw_path = QFileDialog::getOpenFileName(this, tr("Select Skin Texture"), QString(), filter);
    if (raw_path.isEmpty() || !QFileInfo::exists(raw_path)) {
        return;
    }
    QString cooked_path = FS::NormalizePath(raw_path);
    ui->skinPathTextBox->setText(cooked_path);
}

SkinUploadDialog::SkinUploadDialog(MinecraftAccountPtr acct, QWidget* parent) : QDialog(parent), m_acct(acct), ui(new Ui::SkinUploadDialog)
{
    ui->setupUi(this);

    // FIXME: add a model for this, download/refresh the capes on demand
    auto& accountData = *acct->accountData();
    int index = 0;
    ui->capeCombo->addItem(tr("No Cape"), QVariant());
    auto currentCape = accountData.minecraftProfile.currentCape;
    if (currentCape.isEmpty()) {
        ui->capeCombo->setCurrentIndex(index);
    }

    for (auto& cape : accountData.minecraftProfile.capes) {
        index++;
        if (cape.data.size()) {
            QPixmap capeImage;
            if (capeImage.loadFromData(cape.data, "PNG")) {
                QPixmap preview = QPixmap(10, 16);
                QPainter painter(&preview);
                painter.drawPixmap(0, 0, capeImage.copy(1, 1, 10, 16));
                ui->capeCombo->addItem(capeImage, cape.alias, cape.id);
                if (currentCape == cape.id) {
                    ui->capeCombo->setCurrentIndex(index);
                }
                continue;
            }
        }
        ui->capeCombo->addItem(cape.alias, cape.id);
        if (currentCape == cape.id) {
            ui->capeCombo->setCurrentIndex(index);
        }
    }
}
