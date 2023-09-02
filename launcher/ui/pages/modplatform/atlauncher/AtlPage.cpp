// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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
 *      Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 *      Copyright 2021 Philip T <me@phit.link>
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

#include "AtlPage.h"
#include "ui/widgets/ProjectItem.h"
#include "ui_AtlPage.h"

#include "BuildConfig.h"

#include "AtlUserInteractionSupportImpl.h"
#include "modplatform/atlauncher/ATLPackInstallTask.h"
#include "ui/dialogs/NewInstanceDialog.h"

#include <QMessageBox>

AtlPage::AtlPage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), ui(new Ui::AtlPage), dialog(dialog)
{
    ui->setupUi(this);

    filterModel = new Atl::FilterModel(this);
    listModel = new Atl::ListModel(this);
    filterModel->setSourceModel(listModel);
    ui->packView->setModel(filterModel);
    ui->packView->setSortingEnabled(true);

    ui->packView->header()->hide();
    ui->packView->setIndentation(0);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    for (int i = 0; i < filterModel->getAvailableSortings().size(); i++) {
        ui->sortByBox->addItem(filterModel->getAvailableSortings().keys().at(i));
    }
    ui->sortByBox->setCurrentText(filterModel->translateCurrentSorting());

    connect(ui->searchEdit, &QLineEdit::textChanged, this, &AtlPage::triggerSearch);
    connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &AtlPage::onSortingSelectionChanged);
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &AtlPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &AtlPage::onVersionSelectionChanged);

    ui->packView->setItemDelegate(new ProjectItemDelegate(this));
}

AtlPage::~AtlPage()
{
    delete ui;
}

bool AtlPage::shouldDisplay() const
{
    return true;
}

void AtlPage::retranslate()
{
    ui->retranslateUi(this);
}

void AtlPage::openedImpl()
{
    if (!initialized) {
        listModel->request();
        initialized = true;
    }

    suggestCurrent();
}

void AtlPage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }

    if (selectedVersion.isEmpty()) {
        dialog->setSuggestedPack();
        return;
    }

    auto uiSupport = new AtlUserInteractionSupportImpl(this);
    dialog->setSuggestedPack(selected.name, selectedVersion, new ATLauncher::PackInstallTask(uiSupport, selected.name, selectedVersion));

    auto editedLogoName = selected.safeName;
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/images/%1.png").arg(selected.safeName.toLower());
    listModel->getLogo(selected.safeName, url,
                       [this, editedLogoName](QString logo) { dialog->setSuggestedIconFromFile(logo, editedLogoName); });
}

void AtlPage::triggerSearch()
{
    filterModel->setSearchTerm(ui->searchEdit->text());
}

void AtlPage::onSortingSelectionChanged(QString sort)
{
    auto toSet = filterModel->getAvailableSortings().value(sort);
    filterModel->setSorting(toSet);
}

void AtlPage::onSelectionChanged(QModelIndex first, [[maybe_unused]] QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if (!first.isValid()) {
        if (isOpened) {
            dialog->setSuggestedPack();
        }
        return;
    }

    selected = filterModel->data(first, Qt::UserRole).value<ATLauncher::IndexedPack>();

    ui->packDescription->setHtml(selected.description.replace("\n", "<br>"));

    for (const auto& version : selected.versions) {
        ui->versionSelectionBox->addItem(version.version);
    }

    suggestCurrent();
}

void AtlPage::onVersionSelectionChanged(QString version)
{
    if (version.isNull() || version.isEmpty()) {
        selectedVersion = "";
        return;
    }

    selectedVersion = version;
    suggestCurrent();
}
