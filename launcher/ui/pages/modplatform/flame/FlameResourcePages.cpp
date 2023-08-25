// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

#include "FlameResourcePages.h"
#include "ui_ResourcePage.h"

#include "FlameResourceModels.h"
#include "ui/dialogs/ResourceDownloadDialog.h"

namespace ResourceDownload {

static bool isOptedOut(ModPlatform::IndexedVersion const& ver)
{
    return ver.downloadUrl.isEmpty();
}

FlameModPage::FlameModPage(ModDownloadDialog* dialog, BaseInstance& instance) : ModPage(dialog, instance)
{
    m_model = new FlameModModel(instance);
    m_ui->packView->setModel(m_model);

    addSortings();

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's contructor...
    connect(m_ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlameModPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlameModPage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &FlameModPage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

auto FlameModPage::validateVersion(ModPlatform::IndexedVersion& ver,
                                   QString mineVer,
                                   std::optional<ModPlatform::ModLoaderTypes> loaders) const -> bool
{
    return ver.mcVersion.contains(mineVer) && !ver.downloadUrl.isEmpty() &&
           (!loaders.has_value() || !ver.loaders || loaders.value() & ver.loaders);
}

bool FlameModPage::optedOut(ModPlatform::IndexedVersion& ver) const
{
    return isOptedOut(ver);
}

void FlameModPage::openUrl(const QUrl& url)
{
    if (url.scheme().isEmpty()) {
        QString query = url.query(QUrl::FullyDecoded);

        if (query.startsWith("remoteUrl=")) {
            // attempt to resolve url from warning page
            query.remove(0, 10);
            ModPage::openUrl({ QUrl::fromPercentEncoding(query.toUtf8()) });  // double decoding is necessary
            return;
        }
    }

    ModPage::openUrl(url);
}

FlameResourcePackPage::FlameResourcePackPage(ResourcePackDownloadDialog* dialog, BaseInstance& instance)
    : ResourcePackResourcePage(dialog, instance)
{
    m_model = new FlameResourcePackModel(instance);
    m_ui->packView->setModel(m_model);

    addSortings();

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's contructor...
    connect(m_ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlameResourcePackPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlameResourcePackPage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &FlameResourcePackPage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

bool FlameResourcePackPage::optedOut(ModPlatform::IndexedVersion& ver) const
{
    return isOptedOut(ver);
}

void FlameResourcePackPage::openUrl(const QUrl& url)
{
    if (url.scheme().isEmpty()) {
        QString query = url.query(QUrl::FullyDecoded);

        if (query.startsWith("remoteUrl=")) {
            // attempt to resolve url from warning page
            query.remove(0, 10);
            ResourcePackResourcePage::openUrl({ QUrl::fromPercentEncoding(query.toUtf8()) });  // double decoding is necessary
            return;
        }
    }

    ResourcePackResourcePage::openUrl(url);
}

FlameTexturePackPage::FlameTexturePackPage(TexturePackDownloadDialog* dialog, BaseInstance& instance)
    : TexturePackResourcePage(dialog, instance)
{
    m_model = new FlameTexturePackModel(instance);
    m_ui->packView->setModel(m_model);

    addSortings();

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's contructor...
    connect(m_ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlameTexturePackPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlameTexturePackPage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &FlameTexturePackPage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

bool FlameTexturePackPage::optedOut(ModPlatform::IndexedVersion& ver) const
{
    return isOptedOut(ver);
}

void FlameTexturePackPage::openUrl(const QUrl& url)
{
    if (url.scheme().isEmpty()) {
        QString query = url.query(QUrl::FullyDecoded);

        if (query.startsWith("remoteUrl=")) {
            // attempt to resolve url from warning page
            query.remove(0, 10);
            ResourcePackResourcePage::openUrl({ QUrl::fromPercentEncoding(query.toUtf8()) });  // double decoding is necessary
            return;
        }
    }

    TexturePackResourcePage::openUrl(url);
}

// I don't know why, but doing this on the parent class makes it so that
// other mod providers start loading before being selected, at least with
// my Qt, so we need to implement this in every derived class...
auto FlameModPage::shouldDisplay() const -> bool
{
    return true;
}
auto FlameResourcePackPage::shouldDisplay() const -> bool
{
    return true;
}
auto FlameTexturePackPage::shouldDisplay() const -> bool
{
    return true;
}

}  // namespace ResourceDownload
