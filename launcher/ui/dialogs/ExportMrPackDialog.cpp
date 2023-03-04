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
#include "ui/dialogs/ProgressDialog.h"
#include "ui_ExportMrPackDialog.h"

#include <QFileDialog>
#include <QFileSystemModel>
#include <QJsonDocument>
#include <QMessageBox>
#include "FileSystem.h"
#include "MMCZip.h"
#include "modplatform/modrinth/ModrinthPackExportTask.h"

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

ExportMrPackDialog::~ExportMrPackDialog()
{
    delete ui;
}

void ExportMrPackDialog::done(int result)
{
    if (result == Accepted) {
        const QString filename = FS::RemoveInvalidFilenameChars(ui->name->text());
        const QString output =
            QFileDialog::getSaveFileName(this, tr("Export %1").arg(ui->name->text()), FS::PathCombine(QDir::homePath(), filename + ".mrpack"),
                                        "Modrinth pack (*.mrpack *.zip)", nullptr);

        if (output.isEmpty())
            return;

        ModrinthPackExportTask task(ui->name->text(), ui->version->text(), ui->summary->text(), instance, output,
                                    [this](const QString& path) { return proxy->blockedPaths().covers(path); });

        ProgressDialog progress(this);
        progress.setSkipButton(true, tr("Abort"));
        if (progress.execWithTask(&task) != QDialog::Accepted)
            return;
    }

    QDialog::done(result);
}