// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "OptionalModDialog.h"
#include "ui_OptionalModDialog.h"

OptionalModDialog::OptionalModDialog(QWidget* parent, const QStringList& mods) : QDialog(parent), ui(new Ui::OptionalModDialog)
{
    ui->setupUi(this);
    for (const QString& mod : mods) {
        auto item = new QListWidgetItem(mod, ui->list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, mod);
    }

    connect(ui->selectAllButton, &QPushButton::clicked, ui->list, [this] {
        for (int i = 0; i < ui->list->count(); i++)
            ui->list->item(i)->setCheckState(Qt::Checked);
    });
    connect(ui->clearAllButton, &QPushButton::clicked, ui->list, [this] {
        for (int i = 0; i < ui->list->count(); i++)
            ui->list->item(i)->setCheckState(Qt::Unchecked);
    });
    connect(ui->list, &QListWidget::itemActivated, [](QListWidgetItem* item) {
        if (item->checkState() == Qt::Checked)
            item->setCheckState(Qt::Unchecked);
        else
            item->setCheckState(Qt::Checked);
    });
}

OptionalModDialog::~OptionalModDialog()
{
    delete ui;
}

QStringList OptionalModDialog::getResult()
{
    QStringList result;
    result.reserve(ui->list->count());
    for (int i = 0; i < ui->list->count(); i++) {
        auto item = ui->list->item(i);
        if (item->checkState() == Qt::Checked)
            result.append(item->data(Qt::UserRole).toString());
    }
    return result;
}
