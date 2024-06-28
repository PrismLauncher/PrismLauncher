// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

#include "ModPage.h"
#include "ui_ResourcePage.h"

#include <QDesktopServices>
#include <QKeyEvent>
#include <QRegularExpression>

#include <memory>

#include "Application.h"
#include "ResourceDownloadTask.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include "ui/dialogs/ResourceDownloadDialog.h"

namespace ResourceDownload {

ModPage::ModPage(ModDownloadDialog* dialog, BaseInstance& instance) : ResourcePage(dialog, instance)
{
    connect(m_ui->resourceFilterButton, &QPushButton::clicked, this, &ModPage::filterMods);
    connect(m_ui->packView, &QListView::doubleClicked, this, &ModPage::onResourceSelected);
}

void ModPage::setFilterWidget(unique_qobject_ptr<ModFilterWidget>& widget)
{
    if (m_filter_widget)
        disconnect(m_filter_widget.get(), nullptr, nullptr, nullptr);

    auto old = m_ui->splitter->replaceWidget(0, widget.get());
    // because we replaced the widget we also need to delete it
    if (old) {
        delete old;
    }

    m_filter_widget.swap(widget);

    m_filter = m_filter_widget->getFilter();

    connect(m_filter_widget.get(), &ModFilterWidget::filterChanged, this, &ModPage::triggerSearch);
    prepareProviderCategories();
}

/******** Callbacks to events in the UI (set up in the derived classes) ********/

void ModPage::filterMods()
{
    m_filter_widget->setHidden(!m_filter_widget->isHidden());
}

void ModPage::triggerSearch()
{
    auto changed = m_filter_widget->changed();
    m_filter = m_filter_widget->getFilter();
    m_ui->packView->selectionModel()->setCurrentIndex({}, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    m_ui->packView->clearSelection();
    m_ui->packDescription->clear();
    m_ui->versionSelectionBox->clear();
    updateSelectionButton();

    static_cast<ModModel*>(m_model)->searchWithTerm(getSearchTerm(), m_ui->sortByBox->currentData().toUInt(), changed);
    m_fetch_progress.watch(m_model->activeSearchJob().get());
}

QMap<QString, QString> ModPage::urlHandlers() const
{
    QMap<QString, QString> map;
    map.insert(QRegularExpression::anchoredPattern("(?:www\\.)?modrinth\\.com\\/mod\\/([^\\/]+)\\/?"), "modrinth");
    map.insert(QRegularExpression::anchoredPattern("(?:www\\.)?curseforge\\.com\\/minecraft\\/mc-mods\\/([^\\/]+)\\/?"), "curseforge");
    map.insert(QRegularExpression::anchoredPattern("minecraft\\.curseforge\\.com\\/projects\\/([^\\/]+)\\/?"), "curseforge");
    return map;
}

/******** Make changes to the UI ********/

void ModPage::addResourceToPage(ModPlatform::IndexedPack::Ptr pack,
                                ModPlatform::IndexedVersion& version,
                                const std::shared_ptr<ResourceFolderModel> base_model)
{
    bool is_indexed = !APPLICATION->settings()->get("ModMetadataDisabled").toBool();
    m_model->addPack(pack, version, base_model, is_indexed);
}

}  // namespace ResourceDownload
