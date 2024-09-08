// SPDX-FileCopyrightText: 2022-2023 Sefa Eyeoglu <contact@scrumplex.net>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022-2023 Sefa Eyeoglu <contact@scrumplex.net>
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

#pragma once

#include <QMainWindow>
#include <QSortFilterProxyModel>

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class VersionPage;
}

class VersionPage : public QMainWindow, public BasePage {
    Q_OBJECT

   public:
    explicit VersionPage(MinecraftInstance* inst, QWidget* parent = 0);
    virtual ~VersionPage();
    virtual QString displayName() const override { return tr("Version"); }
    virtual QIcon icon() const override;
    virtual QString id() const override { return "version"; }
    virtual QString helpPage() const override { return "Instance-Version"; }
    virtual bool shouldDisplay() const override;
    void retranslate() override;

    void openedImpl() override;
    void closedImpl() override;

   private slots:
    void on_actionChange_version_triggered();
    void on_actionInstall_Loader_triggered();
    void on_actionAdd_Empty_triggered();
    void on_actionReload_triggered();
    void on_actionRemove_triggered();
    void on_actionMove_up_triggered();
    void on_actionMove_down_triggered();
    void on_actionAdd_to_Minecraft_jar_triggered();
    void on_actionReplace_Minecraft_jar_triggered();
    void on_actionImport_Components_triggered();
    void on_actionAdd_Agents_triggered();
    void on_actionRevert_triggered();
    void on_actionEdit_triggered();
    void on_actionCustomize_triggered();
    void on_actionDownload_All_triggered();

    void on_actionMinecraftFolder_triggered();
    void on_actionLibrariesFolder_triggered();

    void updateVersionControls();

   private:
    ComponentPtr current();
    int currentRow();
    void updateButtons(int row = -1);
    void preselect(int row = 0);
    int doUpdate();

   protected:
    QMenu* createPopupMenu() override;

    /// FIXME: this shouldn't be necessary!
    bool reloadPackProfile();

   private:
    Ui::VersionPage* ui;
    QSortFilterProxyModel* m_filterModel;
    std::shared_ptr<PackProfile> m_profile;
    MinecraftInstance* m_inst;
    int currentIdx = 0;

    std::shared_ptr<Setting> m_wide_bar_setting = nullptr;

   public slots:
    void versionCurrent(const QModelIndex& current, const QModelIndex& previous);

   private slots:
    void onGameUpdateError(QString error);
    void packageCurrent(const QModelIndex& current, const QModelIndex& previous);
    void showContextMenu(const QPoint& pos);
    void onFilterTextChanged(const QString& newContents);
};
