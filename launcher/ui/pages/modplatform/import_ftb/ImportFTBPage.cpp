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
#include "ui/widgets/ProjectItem.h"
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
        currentModel = new FilterModel(this);
        listModel = new ListModel(this);
        currentModel->setSourceModel(listModel);

        ui->modpackList->setModel(currentModel);
        ui->modpackList->setSortingEnabled(true);
        ui->modpackList->header()->hide();
        ui->modpackList->setIndentation(0);
        ui->modpackList->setIconSize(QSize(42, 42));

        for (int i = 0; i < currentModel->getAvailableSortings().size(); i++) {
            ui->sortByBox->addItem(currentModel->getAvailableSortings().keys().at(i));
        }

        ui->sortByBox->setCurrentText(currentModel->translateCurrentSorting());
    }

    connect(ui->modpackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &ImportFTBPage::onPublicPackSelectionChanged);

    connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &ImportFTBPage::onSortingSelectionChanged);

    connect(ui->searchEdit, &QLineEdit::textChanged, this, &ImportFTBPage::triggerSearch);

    ui->modpackList->setItemDelegate(new ProjectItemDelegate(this));
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
    Modpack selectedPack = currentModel->data(now, Qt::UserRole).value<Modpack>();
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

void ImportFTBPage::onSortingSelectionChanged(QString sort)
{
    FilterModel::Sorting toSet = currentModel->getAvailableSortings().value(sort);
    currentModel->setSorting(toSet);
}

void ImportFTBPage::triggerSearch()
{
    currentModel->setSearchTerm(ui->searchEdit->text());
}

}  // namespace FTBImportAPP
