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
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlamePackExportTask.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui_ExportMrPackDialog.h"

#include <QFileDialog>
#include <QFileSystemModel>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>
#include "FastFileIconProvider.h"
#include "FileSystem.h"
#include "MMCZip.h"
#include "modplatform/modrinth/ModrinthPackExportTask.h"

ExportMrPackDialog::ExportMrPackDialog(InstancePtr instance, QWidget* parent, ModPlatform::ResourceProvider provider)
    : QDialog(parent), instance(instance), ui(new Ui::ExportMrPackDialog), m_provider(provider)
{
    ui->setupUi(this);
    ui->name->setText(instance->name());
    if (m_provider == ModPlatform::ResourceProvider::MODRINTH) {
        ui->summary->setText(instance->notes().split(QRegularExpression("\\r?\\n"))[0]);
        ui->author->hide();
        ui->authorLabel->hide();
        ui->gnerateModlist->hide();
    } else {
        setWindowTitle("Export CurseForge Pack");
        ui->version->setText("");
        ui->summaryLabel->setText("ProjectID");
    }

    // ensure a valid pack is generated
    // the name and version fields mustn't be empty
    connect(ui->name, &QLineEdit::textEdited, this, &ExportMrPackDialog::validate);
    connect(ui->version, &QLineEdit::textEdited, this, &ExportMrPackDialog::validate);
    // the instance name can technically be empty
    validate();

    QFileSystemModel* model = new QFileSystemModel(this);
    model->setIconProvider(&icons);

    // use the game root - everything outside cannot be exported
    const QDir root(instance->gameRoot());
    proxy = new FileIgnoreProxy(instance->gameRoot(), this);
    proxy->setSourceModel(model);
    proxy->setFilterRegularExpression("^(?!(\\.DS_Store)|([tT]humbs\\.db)).+$");

    const QDir::Filters filter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Hidden);

    for (const QString& file : root.entryList(filter)) {
        if (!(file == "mods" || file == "coremods" || file == "datapacks" || file == "config" || file == "options.txt" ||
              file == "servers.dat"))
            proxy->blockedPaths().insert(file);
    }

    MinecraftInstance* mcInstance = dynamic_cast<MinecraftInstance*>(instance.get());
    if (mcInstance) {
        const QDir index = mcInstance->loaderModList()->indexDir();
        if (index.exists())
            proxy->blockedPaths().insert(root.relativeFilePath(index.absolutePath()));
    }

    ui->treeView->setModel(proxy);
    ui->treeView->setRootIndex(proxy->mapFromSource(model->index(instance->gameRoot())));
    ui->treeView->sortByColumn(0, Qt::AscendingOrder);

    model->setFilter(filter);
    model->setRootPath(instance->gameRoot());

    QHeaderView* headerView = ui->treeView->header();
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
        QString output;
        if (m_provider == ModPlatform::ResourceProvider::MODRINTH)
            output = QFileDialog::getSaveFileName(this, tr("Export %1").arg(ui->name->text()),
                                                  FS::PathCombine(QDir::homePath(), filename + ".mrpack"), "Modrinth pack (*.mrpack *.zip)",
                                                  nullptr);
        else
            output = QFileDialog::getSaveFileName(this, tr("Export %1").arg(ui->name->text()),
                                                  FS::PathCombine(QDir::homePath(), filename + ".zip"), "Curseforge pack (*.zip)", nullptr);

        if (output.isEmpty())
            return;
        Task* task;
        if (m_provider == ModPlatform::ResourceProvider::MODRINTH)
            task = new ModrinthPackExportTask(ui->name->text(), ui->version->text(), ui->summary->text(), instance, output,
                                              [this](const QString& path) { return proxy->blockedPaths().covers(path); });
        else
            task = new FlamePackExportTask(ui->name->text(), ui->version->text(), ui->author->text(), ui->summary->text(),
                                           ui->gnerateModlist->isChecked(), instance, output,
                                           [this](const QString& path) { return proxy->blockedPaths().covers(path); });

        connect(task, &Task::failed,
                [this](const QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
        connect(task, &Task::aborted, [this] {
            CustomMessageBox::selectable(this, tr("Task aborted"), tr("The task has been aborted by the user."), QMessageBox::Information)
                ->show();
        });
        connect(task, &Task::finished, [task] { task->deleteLater(); });

        ProgressDialog progress(this);
        progress.setSkipButton(true, tr("Abort"));
        if (progress.execWithTask(task) != QDialog::Accepted)
            return;
    }

    QDialog::done(result);
}

void ExportMrPackDialog::validate()
{
    const bool invalid =
        ui->name->text().isEmpty() || ((m_provider == ModPlatform::ResourceProvider::MODRINTH) && ui->version->text().isEmpty());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(invalid);
}
