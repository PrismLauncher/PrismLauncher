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

#include "ModrinthModPage.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "ui_ModPage.h"

#include "ModrinthModModel.h"
#include "ui/dialogs/ModDownloadDialog.h"

ModrinthModPage::ModrinthModPage(ModDownloadDialog* dialog, BaseInstance* instance)
    : ModPage(dialog, instance, new ModrinthAPI())
{
    listModel = new Modrinth::ListModel(this);
    ui->packView->setModel(listModel);

    // index is used to set the sorting with the modrinth api
    ui->sortByBox->addItem(tr("Sort by Relevance"));
    ui->sortByBox->addItem(tr("Sort by Downloads"));
    ui->sortByBox->addItem(tr("Sort by Follows"));
    ui->sortByBox->addItem(tr("Sort by Last Updated"));
    ui->sortByBox->addItem(tr("Sort by Newest"));

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems, 
    // so it's best not to connect them in the parent's constructor...
    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthModPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &ModrinthModPage::onVersionSelectionChanged);
    connect(ui->modSelectionButton, &QPushButton::clicked, this, &ModrinthModPage::onModSelected);
}

auto ModrinthModPage::validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, ModAPI::ModLoaderType loader) const -> bool
{
    auto loaderStrings = ModrinthAPI::getModLoaderStrings(loader);

    auto loaderCompatible = false;
    for (auto remoteLoader : ver.loaders)
    {
        if (loaderStrings.contains(remoteLoader)) {
            loaderCompatible = true;
            break;
        }
    }
    return ver.mcVersion.contains(mineVer) && loaderCompatible;
}

// I don't know why, but doing this on the parent class makes it so that
// other mod providers start loading before being selected, at least with
// my Qt, so we need to implement this in every derived class...
auto ModrinthModPage::shouldDisplay() const -> bool { return true; }
