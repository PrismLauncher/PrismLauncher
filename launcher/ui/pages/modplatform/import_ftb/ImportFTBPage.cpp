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

#include <QFileDialog>
#include <QFileInfo>
#include <QTemporaryFile>
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

    connect(ui->browseButton, &QPushButton::clicked, this, [this] {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select FTBApp instances directory"), listModel->getUserPath(),
                                                        QFileDialog::ShowDirsOnly);
        if (!dir.isEmpty())
            listModel->setPath(dir);
    });

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

QString saveIconToTempFile(const QIcon& icon)
{
    if (icon.isNull()) {
        return QString();
    }

    QPixmap pixmap = icon.pixmap(icon.availableSizes().last());
    if (pixmap.isNull()) {
        return QString();
    }

    QTemporaryFile tempFile(QDir::tempPath() + "/iconXXXXXX.png");
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        return QString();
    }

    QString tempPath = tempFile.fileName();
    tempFile.close();

    if (!pixmap.save(tempPath, "PNG")) {
        QFile::remove(tempPath);
        return QString();
    }

    return tempPath;  // Success
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
    QString editedLogoName = QString("ftb_%1_%2.jpg").arg(selected.name, QString::number(selected.id));
    auto iconPath = FS::PathCombine(selected.path, "folder.jpg");
    if (!QFileInfo::exists(iconPath)) {
        // need to save the icon as that actual logo is not a image on the disk
        iconPath = saveIconToTempFile(selected.icon);
    }
    if (!iconPath.isEmpty() && QFileInfo::exists(iconPath)) {
        dialog->setSuggestedIconFromFile(iconPath, editedLogoName);
    }
}

void ImportFTBPage::onPublicPackSelectionChanged(QModelIndex now, QModelIndex)
{
    if (!now.isValid()) {
        onPackSelectionChanged();
        return;
    }

    QVariant raw = currentModel->data(now, Qt::UserRole);
    Q_ASSERT(raw.canConvert<Modpack>());
    auto selectedPack = raw.value<Modpack>();
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

void ImportFTBPage::setSearchTerm(QString term)
{
    ui->searchEdit->setText(term);
}

QString ImportFTBPage::getSerachTerm() const
{
    return ui->searchEdit->text();
}
}  // namespace FTBImportAPP
