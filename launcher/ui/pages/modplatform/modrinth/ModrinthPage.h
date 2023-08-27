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

#include "Application.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/pages/BasePage.h"

#include "modplatform/modrinth/ModrinthPackManifest.h"
#include "ui/widgets/ProgressWidget.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class ModrinthPage;
}

namespace Modrinth {
class ModpackListModel;
}

class ModrinthPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit ModrinthPage(NewInstanceDialog* dialog, QWidget* parent = nullptr);
    ~ModrinthPage() override;

    QString displayName() const override { return tr("Modrinth"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("modrinth"); }
    QString id() const override { return "modrinth"; }
    QString helpPage() const override { return "Modrinth-platform"; }

    inline auto debugName() const -> QString { return "Modrinth"; }
    inline auto metaEntryBase() const -> QString { return "ModrinthModpacks"; };

    auto getCurrent() -> Modrinth::Modpack& { return current; }
    void suggestCurrent();

    void updateUI();

    void retranslate() override;
    void openedImpl() override;
    bool eventFilter(QObject* watched, QEvent* event) override;

   private slots:
    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(QString data);
    void triggerSearch();

   private:
    Ui::ModrinthPage* ui;
    NewInstanceDialog* dialog;
    Modrinth::ModpackListModel* m_model;

    Modrinth::Modpack current;
    QString selectedVersion;

    ProgressWidget m_fetch_progress;

    // Used to do instant searching with a delay to cache quick changes
    QTimer m_search_timer;
};
