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

#include "ExportMrPackDialog.h"
#include <QFileSystemModel>
#include "ui_ExportMrPackDialog.h"

ExportMrPackDialog::ExportMrPackDialog(InstancePtr instance, QWidget* parent)
    : QDialog(parent), instance(instance), ui(new Ui::ExportMrPackDialog)
{
    ui->setupUi(this);
    ui->name->setText(instance->name());

    auto model = new QFileSystemModel(this);
    // use the game root - everything outside cannot be exported
    QString root = instance->gameRoot();
    proxy = new PackIgnoreProxy(root, this);
    proxy->setSourceModel(model);
    ui->treeView->setModel(proxy);
    ui->treeView->setRootIndex(proxy->mapFromSource(model->index(root)));
    ui->treeView->sortByColumn(0, Qt::AscendingOrder);
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Hidden);
    model->setRootPath(root);
    auto headerView = ui->treeView->header();
    headerView->setSectionResizeMode(QHeaderView::ResizeToContents);
    headerView->setSectionResizeMode(0, QHeaderView::Stretch);
}

void ExportMrPackDialog::done(int result)
{
    if (result != Accepted) {
        QDialog::done(result);
        return;
    }
    QDialog::done(result);
}

ExportMrPackDialog::~ExportMrPackDialog()
{
    delete ui;
}
