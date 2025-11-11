// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
 *      Copyright 2021-2022 kb1000
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

#pragma once

#include "modplatform/ModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "ui/dialogs/NewInstanceDialog.h"

#include "ui/pages/modplatform/ModpackProviderBasePage.h"
#include "ui/widgets/ModFilterWidget.h"
#include "ui/widgets/ProgressWidget.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class ModrinthPage;
}

namespace Modrinth {
class ModpackListModel;
}

class ModrinthPage : public QWidget, public ModpackProviderBasePage {
    Q_OBJECT

   public:
    explicit ModrinthPage(NewInstanceDialog* dialog, QWidget* parent = nullptr);
    ~ModrinthPage() override;

    QString displayName() const override { return tr("Modrinth"); }
    QIcon icon() const override { return QIcon::fromTheme("modrinth"); }
    QString id() const override { return "modrinth"; }
    QString helpPage() const override { return "Modrinth-platform"; }

    inline QString debugName() const { return "Modrinth"; }
    inline QString metaEntryBase() const { return "ModrinthModpacks"; };

    ModPlatform::IndexedPack::Ptr getCurrent() { return m_current; }
    void suggestCurrent();

    void updateUI();

    void retranslate() override;
    void openedImpl() override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    /** Programatically set the term in the search bar. */
    virtual void setSearchTerm(QString) override;
    /** Get the current term in the search bar. */
    virtual QString getSerachTerm() const override;

   private slots:
    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(int index);
    void triggerSearch();
    void createFilterWidget();

   private:
    Ui::ModrinthPage* m_ui;
    NewInstanceDialog* m_dialog;
    Modrinth::ModpackListModel* m_model;

    ModPlatform::IndexedPack::Ptr m_current;
    QString m_selectedVersion;

    ProgressWidget m_fetch_progress;

    // Used to do instant searching with a delay to cache quick changes
    QTimer m_search_timer;

    std::unique_ptr<ModFilterWidget> m_filterWidget;
    Task::Ptr m_categoriesTask;

    ModrinthAPI m_api;
    Task::Ptr m_job;
    Task::Ptr m_job2;
};
