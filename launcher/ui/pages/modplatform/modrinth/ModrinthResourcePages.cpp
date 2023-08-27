// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "ModrinthResourcePages.h"
#include "ui_ResourcePage.h"

#include "modplatform/modrinth/ModrinthAPI.h"

#include "ui/dialogs/ResourceDownloadDialog.h"

#include "ui/pages/modplatform/modrinth/ModrinthResourceModels.h"

namespace ResourceDownload {

ModrinthModPage::ModrinthModPage(ModDownloadDialog* dialog, BaseInstance& instance) : ModPage(dialog, instance)
{
    m_model = new ModrinthModModel(instance);
    m_ui->packView->setModel(m_model);

    addSortings();

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's constructor...
    connect(m_ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthModPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &ModrinthModPage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &ModrinthModPage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

auto ModrinthModPage::validateVersion(ModPlatform::IndexedVersion& ver,
                                      QString mineVer,
                                      std::optional<ModPlatform::ModLoaderTypes> loaders) const -> bool
{
    return ver.mcVersion.contains(mineVer) && (!loaders.has_value() || !ver.loaders || loaders.value() & ver.loaders);
}

ModrinthResourcePackPage::ModrinthResourcePackPage(ResourcePackDownloadDialog* dialog, BaseInstance& instance)
    : ResourcePackResourcePage(dialog, instance)
{
    m_model = new ModrinthResourcePackModel(instance);
    m_ui->packView->setModel(m_model);

    addSortings();

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's constructor...
    connect(m_ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthResourcePackPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &ModrinthResourcePackPage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &ModrinthResourcePackPage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

ModrinthTexturePackPage::ModrinthTexturePackPage(TexturePackDownloadDialog* dialog, BaseInstance& instance)
    : TexturePackResourcePage(dialog, instance)
{
    m_model = new ModrinthTexturePackModel(instance);
    m_ui->packView->setModel(m_model);

    addSortings();

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's constructor...
    connect(m_ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthTexturePackPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &ModrinthTexturePackPage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &ModrinthTexturePackPage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

ModrinthShaderPackPage::ModrinthShaderPackPage(ShaderPackDownloadDialog* dialog, BaseInstance& instance)
    : ShaderPackResourcePage(dialog, instance)
{
    m_model = new ModrinthShaderPackModel(instance);
    m_ui->packView->setModel(m_model);

    addSortings();

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's constructor...
    connect(m_ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthShaderPackPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &ModrinthShaderPackPage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &ModrinthShaderPackPage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

// I don't know why, but doing this on the parent class makes it so that
// other mod providers start loading before being selected, at least with
// my Qt, so we need to implement this in every derived class...
auto ModrinthModPage::shouldDisplay() const -> bool
{
    return true;
}
auto ModrinthResourcePackPage::shouldDisplay() const -> bool
{
    return true;
}
auto ModrinthTexturePackPage::shouldDisplay() const -> bool
{
    return true;
}
auto ModrinthShaderPackPage::shouldDisplay() const -> bool
{
    return true;
}

}  // namespace ResourceDownload
