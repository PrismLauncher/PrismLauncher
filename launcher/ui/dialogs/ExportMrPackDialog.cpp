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
#include <QJsonDocument>
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui_ExportMrPackDialog.h"

#include <QFileDialog>
#include <QFileSystemModel>
#include <QMessageBox>
#include "MMCZip.h"

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
    if (result == Accepted)
        runExport();

    QDialog::done(result);
}

void ExportMrPackDialog::runExport()
{
    const QString filename = FS::RemoveInvalidFilenameChars(ui->name->text());
    const QString output =
        QFileDialog::getSaveFileName(this, tr("Export %1").arg(ui->name->text()), FS::PathCombine(QDir::homePath(), filename + ".mrpack"),
                                     "Modrinth modpack (*.mrpack *.zip)", nullptr);

    if (output.isEmpty())
        return;

    QFileInfoList files;
    if (!MMCZip::collectFileListRecursively(instance->gameRoot(), nullptr, &files,
                                            [this](const QString& path) { return proxy->blockedPaths().covers(path); })) {
        QMessageBox::warning(this, tr("Unable to export modpack"), tr("Could not collect list of files"));
        return;
    }

    QuaZip zip(output);
    if (!zip.open(QuaZip::mdCreate)) {
        QFile::remove(output);
        QMessageBox::warning(this, tr("Unable to export modpack"), tr("Could not create file"));
        return;
    }

    QuaZipFile indexFile(&zip);
    indexFile.setFileName("modrinth.index.json");
    if (!indexFile.open(QuaZipFile::NewOnly)) {
        QFile::remove(output);
        QMessageBox::warning(this, tr("Unable to export modpack"), tr("Could not create index"));
        return;
    }
    indexFile.write(generateIndex());
    indexFile.close();

    // should exist
    QDir dotMinecraft(instance->gameRoot());

    for (const QFileInfo& file : files)
        if (!JlCompress::compressFile(&zip, file.absoluteFilePath(), "overrides/" + dotMinecraft.relativeFilePath(file.absoluteFilePath())))
            QMessageBox::warning(this, tr("Unable to export modpack"), tr("Could not compress %1").arg(file.absoluteFilePath()));

    zip.close();

    if (zip.getZipError() != 0) {
        QFile::remove(output);
        QMessageBox::warning(this, tr("Unable to export modpack"), tr("A zip error occured"));
        return;
    }
}

QByteArray ExportMrPackDialog::generateIndex()
{
    QJsonObject obj;
    obj["formatVersion"] = 1;
    obj["game"] = "minecraft";
    obj["name"] = ui->name->text();
    obj["versionId"] = ui->version->text();
    obj["summary"] = ui->summary->text();

    MinecraftInstance* mc = dynamic_cast<MinecraftInstance*>(instance.get());
    if (mc) {
        auto profile = mc->getPackProfile();
        auto minecraft = profile->getComponent("net.minecraft");

        QJsonObject dependencies;
        dependencies["minecraft"] = minecraft->m_version;
        obj["dependencies"] = dependencies;
    }

    return QJsonDocument(obj).toJson();
}