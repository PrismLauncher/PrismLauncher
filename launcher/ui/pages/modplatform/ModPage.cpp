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

#include <QRegularExpression>

#include <memory>
#include <utility>

#include "Application.h"
#include "ui/dialogs/ResourceDownloadDialog.h"

namespace {
ResourceDownload::ResourceDescriptor prepareModDescriptor()
{
    QMap<QString, QString> urlHandlers;
    urlHandlers.insert(QRegularExpression::anchoredPattern(R"((?:www\.)?modrinth\.com\/mod\/([^\/]+)\/?)"), "modrinth");
    urlHandlers.insert(QRegularExpression::anchoredPattern(R"((?:www\.)?curseforge\.com\/minecraft\/mc-mods\/([^\/]+)\/?)"), "curseforge");
    urlHandlers.insert(QRegularExpression::anchoredPattern(R"(minecraft\.curseforge\.com\/projects\/([^\/]+)\/?)"), "curseforge");
    return {
        .helpPage = "Mod-platform",
        //: The singular version of 'mods'
        .resourceString = QObject::tr("mod"),
        //: The plural version of 'mod'
        .resourcesString = QObject::tr("mods"),
        .supportsFiltering = true,
        .isIndexed = true,
        .urlHandlers = urlHandlers,
    };
}
}  // namespace

namespace ResourceDownload {
ModPage::ModPage(ModDownloadDialog* dialog, BaseInstance& instance, ResourceProviderData p, ResourceAPI* api, ModFilterWidget* filterWidget)
    : ResourcePage(dialog, instance, prepareModDescriptor(), std::move(p)), m_api(api)
{
    auto* model = new ModModel(instance, api, debugName(), metaEntryBase());
    m_model = model;
    m_ui->packView->setModel(m_model);

    addSortings();

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems,
    // so it's best not to connect them in the parent's constructor...
    setFilterWidget(filterWidget);
    model->setFilter(getFilter());

    connect(m_model, &ResourceModel::versionListUpdated, this, &ResourcePage::versionListUpdated);
    connect(m_model, &ResourceModel::projectInfoUpdated, this, &ResourcePage::updateUi);
    connect(m_model, &QAbstractListModel::modelReset, this, &ResourcePage::modelReset);

    connect(m_ui->sortByBox, &QComboBox::currentIndexChanged, this, &ModPage::triggerSearch);
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentIndexChanged, this, &ModPage::onVersionSelectionChanged);
    connect(m_ui->resourceSelectionButton, &QPushButton::clicked, this, &ModPage::onResourceSelected);

    m_ui->packDescription->setMetaEntry(metaEntryBase());
    connect(m_ui->resourceFilterButton, &QPushButton::clicked, this, &ModPage::filterMods);
}

void ModPage::setFilterWidget(ModFilterWidget* widget)
{
    if (m_filter_widget) {
        disconnect(m_filter_widget.get(), nullptr, nullptr, nullptr);
    }

    auto* old = m_ui->splitter->replaceWidget(0, widget);
    // because we replaced the widget we also need to delete it
    if (old) {
        old->deleteLater();
    }

    m_filter_widget.reset(widget);

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
    m_fetchProgress.watch(m_model->activeSearchJob().get());
}

void ModPage::prepareProviderCategories()
{
    auto [task, response] = m_api->getModCategories();
    m_categoriesTask = task;
    connect(m_categoriesTask.get(), &Task::succeeded, [this, response]() {
        auto categories = m_api->loadModCategories(*response);
        m_filter_widget->setCategories(categories);
    });
    m_categoriesTask->start();
};
}  // namespace ResourceDownload
