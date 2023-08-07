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

#include "ImportFTBPage.h"
#include "ui_ImportFTBPage.h"

#include <QWidget>
#include "FileSystem.h"
#include "ListModel.h"
#include "modplatform/import_ftb/PackInstallTask.h"
#include "ui/dialogs/NewInstanceDialog.h"

namespace FTBImportAPP {

ImportFTBPage::ImportFTBPage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), dialog(dialog), ui(new Ui::ImportFTBPage)
{
    ui->setupUi(this);

    {
        listModel = new ListModel(this);

        ui->modpackList->setModel(listModel);
        ui->modpackList->setSortingEnabled(true);
        ui->modpackList->header()->hide();
        ui->modpackList->setIndentation(0);
        ui->modpackList->setIconSize(QSize(42, 42));
    }

    connect(ui->modpackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &ImportFTBPage::onPublicPackSelectionChanged);

    ui->modpackList->selectionModel()->reset();
}

ImportFTBPage::~ImportFTBPage()
{
    delete ui;
}

void ImportFTBPage::openedImpl()
{
    if (!initialized) {
        listModel->update();
        initialized = true;
    }
    suggestCurrent();
}

void ImportFTBPage::retranslate()
{
    ui->retranslateUi(this);
}

void ImportFTBPage::suggestCurrent()
{
    if (!isOpened)
        return;

    if (selected.path.isEmpty()) {
        dialog->setSuggestedPack();
        return;
    }

    dialog->setSuggestedPack(selected.name, new PackInstallTask(selected));
    QString editedLogoName = QString("ftb_%1").arg(selected.id);
    dialog->setSuggestedIconFromFile(FS::PathCombine(selected.path, "folder.jpg"), editedLogoName);
}

void ImportFTBPage::onPublicPackSelectionChanged(QModelIndex now, QModelIndex prev)
{
    if (!now.isValid()) {
        onPackSelectionChanged();
        return;
    }
    Modpack selectedPack = listModel->data(now, Qt::UserRole).value<Modpack>();
    onPackSelectionChanged(&selectedPack);
}

void ImportFTBPage::onPackSelectionChanged(Modpack* pack)
{
    if (pack) {
        selected = *pack;
        suggestCurrent();
        return;
    }
    if (isOpened)
        dialog->setSuggestedPack();
}

}  // namespace FTBImportAPP
