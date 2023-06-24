// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
 */

#include <quazip.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QValidator>

#include "Application.h"
#include "CreateEmptyResourcePackDialog.h"
#include "minecraft/AssetsUtils.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ResourcePack.h"
#include "ui_CreateEmptyResourcePackDialog.h"

CreateEmptyResourcePackDialog::CreateEmptyResourcePackDialog(MinecraftInstance* instance, QWidget* parent)
    : QDialog(parent), instance(instance), ui(new Ui::CreateEmptyResourcePackDialog)
{
    ui->setupUi(this);

    int format = defaultPackFormat();
    std::pair<Version, Version> range = ResourcePack::formatCompatibleVersions(format);

    ui->format->setValidator(new QIntValidator(this));
    ui->format->setPlaceholderText(QString("%1 (%2 - %3)").arg(QString::number(format), range.first.toString(), range.second.toString()));

    connect(ui->name, &QLineEdit::textChanged, this, [this] {
        const bool valid = !ui->name->text().isEmpty();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
    });
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui->browse, &QPushButton::clicked, this, [this] {
        const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        const QString filter = QMimeDatabase().mimeTypeForName("image/png").filterString();
        const QString path = QFileDialog::getOpenFileName(this, tr("Select Icon"), pictures, filter);

        if (path.isNull())
            return;
        if (!QFileInfo(path).isReadable()) {
            QMessageBox(QMessageBox::Critical, tr("Invalid icon"), tr("I cannot read the icon you chose."), QMessageBox::Ok, this).exec();
            return;
        }

        icon = path;
        ui->removeIcon->setEnabled(true);
    });
    connect(ui->removeIcon, &QPushButton::clicked, this, [this] {
        icon = QString();
        ui->removeIcon->setEnabled(false);
    });

    setMaximumHeight(sizeHint().height());
}

CreateEmptyResourcePackDialog::~CreateEmptyResourcePackDialog()
{
    delete ui;
}

void CreateEmptyResourcePackDialog::done(int result)
{
    QDialog::done(result);

    if (result == QDialog::Accepted)
        create();
}

void CreateEmptyResourcePackDialog::create()
{
    QDir output(instance->resourcePacksDir() + '/' + ui->name->text());
    if (output.exists() || !output.mkpath(".")) {
        QMessageBox(QMessageBox::Critical, tr("Failed to create folder"), tr("The pack's folder couldn't be created."), QMessageBox::Ok,
                    parentWidget())
            .exec();
        return;
    }

    {
        QFile metaFile(output.filePath("pack.mcmeta"));
        if (!metaFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox(QMessageBox::Critical, tr("Failed to open meta"), tr("Couldn't open the pack.mcmeta file."), QMessageBox::Ok,
                        parentWidget())
                .exec();
            output.removeRecursively();
            return;
        }

        QJsonObject pack;
        pack["description"] = ui->description->text();
        pack["pack_format"] = packFormat();

        metaFile.write(QJsonDocument(QJsonObject{ { "pack", pack } }).toJson());
    }

    if (!icon.isNull() && !QFile::copy(icon, output.filePath("pack.png"))) {
        // soft warning
        QMessageBox(QMessageBox::Warning, tr("Failed to save icon"), tr("Couldn't copy %1 to pack.png.").arg(icon), QMessageBox::Ok,
                    parentWidget())
            .exec();
    }

    if (!output.mkpath("assets/minecraft")) {
        QMessageBox(QMessageBox::Warning, tr("Failed to create folder"), tr("assets/minecraft couldn't be created.").arg(icon), QMessageBox::Ok,
                    parentWidget())
            .exec();
    }
}

int CreateEmptyResourcePackDialog::packFormat()
{
    if (!ui->format->text().isEmpty())
        return ui->format->text().toInt();

    return defaultPackFormat();
}

int CreateEmptyResourcePackDialog::defaultPackFormat()
{
    return ResourcePack::fuzzyFormat(instance->getPackProfile()->getComponentVersion("net.minecraft"));
}
