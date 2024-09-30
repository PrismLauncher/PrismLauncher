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

#include "AtlFilterModel.h"
#include "AtlListModel.h"

#include <modplatform/atlauncher/ATLPackInstallTask.h>
#include <QWidget>

#include "Application.h"
#include "ui/pages/modplatform/ModpackProviderBasePage.h"

namespace Ui {
class AtlPage;
}

class NewInstanceDialog;

class AtlPage : public QWidget, public ModpackProviderBasePage {
    Q_OBJECT

   public:
    explicit AtlPage(NewInstanceDialog* dialog, QWidget* parent = 0);
    virtual ~AtlPage();
    virtual QString displayName() const override { return "ATLauncher"; }
    virtual QIcon icon() const override { return APPLICATION->getThemedIcon("atlauncher"); }
    virtual QString id() const override { return "atl"; }
    virtual QString helpPage() const override { return "ATL-platform"; }
    virtual bool shouldDisplay() const override;
    void retranslate() override;

    void openedImpl() override;

    /** Programatically set the term in the search bar. */
    virtual void setSearchTerm(QString) override;
    /** Get the current term in the search bar. */
    [[nodiscard]] virtual QString getSerachTerm() const override;

   private:
    void suggestCurrent();

   private slots:
    void triggerSearch();

    void onSortingSelectionChanged(QString data);

    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(QString data);

   private:
    Ui::AtlPage* ui = nullptr;
    NewInstanceDialog* dialog = nullptr;
    Atl::ListModel* listModel = nullptr;
    Atl::FilterModel* filterModel = nullptr;

    ATLauncher::IndexedPack selected;
    QString selectedVersion;

    bool initialized = false;
};
