// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *      Copyright 2013-2021 MultiMC Contributors
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

#include "FlameModPage.h"
#include "ui_ModPage.h"

#include "FlameModModel.h"
#include "ui/dialogs/ModDownloadDialog.h"

FlameModPage::FlameModPage(ModDownloadDialog* dialog, BaseInstance* instance) 
    : ModPage(dialog, instance, new FlameAPI())
{
    listModel = new FlameMod::ListModel(this);
    ui->packView->setModel(listModel);

    // index is used to set the sorting with the flame api
    ui->sortByBox->addItem(tr("Sort by Featured"));
    ui->sortByBox->addItem(tr("Sort by Popularity"));
    ui->sortByBox->addItem(tr("Sort by last updated"));
    ui->sortByBox->addItem(tr("Sort by Name"));
    ui->sortByBox->addItem(tr("Sort by Author"));
    ui->sortByBox->addItem(tr("Sort by Downloads"));

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems, 
    // so it's best not to connect them in the parent's contructor...
    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlameModPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlameModPage::onVersionSelectionChanged);
    connect(ui->modSelectionButton, &QPushButton::clicked, this, &FlameModPage::onModSelected);
}

auto FlameModPage::validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, QString loaderVer) const -> bool
{
    (void) loaderVer;
    return ver.mcVersion.contains(mineVer);
}

// We override this so that it refreshes correctly, otherwise it wouldn't show
// any mod on the mod list, because the CF API does not support it :(
void FlameModPage::filterMods()
{
    filter_dialog.execWithInstance(static_cast<MinecraftInstance*>(m_instance));

    int prev_size = m_filter->versions.size();
    m_filter = filter_dialog.getFilter();
    int new_size = m_filter->versions.size();

    if(new_size <= 1 && new_size != prev_size)
        listModel->refresh();

    if(ui->versionSelectionBox->count() > 0){
        ui->versionSelectionBox->clear();
        updateModVersions();
    }
}

// I don't know why, but doing this on the parent class makes it so that
// other mod providers start loading before being selected, at least with
// my Qt, so we need to implement this in every derived class...
auto FlameModPage::shouldDisplay() const -> bool { return true; }
