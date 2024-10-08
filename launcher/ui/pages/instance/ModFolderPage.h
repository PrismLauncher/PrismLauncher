// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#pragma once

#include "ExternalResourcesPage.h"

class ModFolderPage : public ExternalResourcesPage {
    Q_OBJECT

   public:
    explicit ModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> model, QWidget* parent = nullptr);
    virtual ~ModFolderPage() = default;

    void setFilter(const QString& filter) { m_fileSelectionFilter = filter; }

    virtual QString displayName() const override { return tr("Mods"); }
    virtual QIcon icon() const override { return APPLICATION->getThemedIcon("loadermods"); }
    virtual QString id() const override { return "mods"; }
    virtual QString helpPage() const override { return "Loader-mods"; }

    virtual bool shouldDisplay() const override;

   public slots:
    void updateFrame(const QModelIndex& current, const QModelIndex& previous) override;

   private slots:
    void removeItems(const QItemSelection& selection) override;

    void downloadMods();
    void updateMods(bool includeDeps = false);
    void deleteModMetadata();
    void exportModMetadata();
    void changeModVersion();

   protected:
    std::shared_ptr<ModFolderModel> m_model;
};

class CoreModFolderPage : public ModFolderPage {
    Q_OBJECT
   public:
    explicit CoreModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> mods, QWidget* parent = 0);
    virtual ~CoreModFolderPage() = default;

    virtual QString displayName() const override { return tr("Core mods"); }
    virtual QIcon icon() const override { return APPLICATION->getThemedIcon("coremods"); }
    virtual QString id() const override { return "coremods"; }
    virtual QString helpPage() const override { return "Core-mods"; }

    virtual bool shouldDisplay() const override;
};

class NilModFolderPage : public ModFolderPage {
    Q_OBJECT
   public:
    explicit NilModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> mods, QWidget* parent = 0);
    virtual ~NilModFolderPage() = default;

    virtual QString displayName() const override { return tr("Nilmods"); }
    virtual QIcon icon() const override { return APPLICATION->getThemedIcon("coremods"); }
    virtual QString id() const override { return "nilmods"; }
    virtual QString helpPage() const override { return "Nilmods"; }

    virtual bool shouldDisplay() const override;
};
