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

#include <LoggedProcess.h>
#include "minecraft/MinecraftInstance.h"
#include "ui/pages/BasePage.h"

#include "settings/Setting.h"

class MultiWorldList;
namespace Ui {
class MultiWorldListPage;
}

class MultiWorldListPage : public QMainWindow, public BasePage {
    Q_OBJECT

   public:
    explicit MultiWorldListPage(MultiWorldList* worlds, QWidget* parent = 0);
    virtual ~MultiWorldListPage();

    virtual QString displayName() const override { return tr("Worlds"); }
    virtual QIcon icon() const override { return QIcon::fromTheme("worlds"); }
    virtual QString id() const override { return "worlds"; }
    virtual QString helpPage() const override { return "Worlds"; }
    virtual bool shouldDisplay() const override;
    void retranslate() override;

    virtual void openedImpl() override;
    virtual void closedImpl() override;

   signals:
    void worldJoined(BaseInstance* instance);

   protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;
    bool worldListFilter(QKeyEvent* ev);
    QMenu* createPopupMenu() override;

   private:
    QModelIndex getSelectedWorld();
    bool isWorldSafe(QModelIndex index);
    bool worldSafetyNagQuestion(const QString& actionType);
    void mceditError();
    void join(const QModelIndex& index, LaunchMode launchMode);
    MinecraftInstance* selectInstance(const QString& message, const BaseInstance* preselectedInstance = nullptr);

   private:
    Ui::MultiWorldListPage* ui;
    MultiWorldList* m_worlds;
    unique_qobject_ptr<LoggedProcess> m_mceditProcess;
    bool m_mceditStarting = false;

    std::shared_ptr<Setting> m_wide_bar_setting = nullptr;
    std::unique_ptr<DataPackFolderModel> m_datapackModel;

   private slots:
    void on_actionCopy_Seed_triggered();
    void on_actionMCEdit_triggered();
    void on_actionRemove_triggered();
    void on_actionAdd_triggered();
    void on_actionCopy_triggered();
    void on_actionRename_triggered();
    void on_actionInstance_Settings_triggered();
    void on_actionRefresh_triggered();
    void on_actionView_Folder_triggered();
    void on_actionData_Packs_triggered();
    void on_actionReset_Icon_triggered();
    void worldChanged(const QModelIndex& current, const QModelIndex& previous);
    void mceditState(LoggedProcess::State state);
    void on_actionJoin_triggered();
    void on_actionJoin_Offline_triggered();
    void worldDoubleClicked(const QModelIndex& index);
    void fileDropped(const QFileInfo& worldInfo);

    void ShowContextMenu(const QPoint& pos);
};
