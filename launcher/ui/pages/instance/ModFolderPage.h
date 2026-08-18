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

#include <QPointer>
#include <QMap>
#include <QSet>
#include <QTabBar>
#include <QToolButton>
#include "ExternalResourcesPage.h"
#include "minecraft/mod/ModProfileKeys.h"
#include "ui/dialogs/ResourceDownloadDialog.h"

class ModFolderPage : public ExternalResourcesPage {
    Q_OBJECT

     inline bool handleNoModLoader();

   public:
    explicit ModFolderPage(MinecraftInstance* inst, ModFolderModel* model, QWidget* parent = nullptr);
    virtual ~ModFolderPage();

     void setFilter(const QString& filter) { m_fileSelectionFilter = filter; }

     virtual QString displayName() const override { return tr("Mods"); }
     virtual QIcon icon() const override { return QIcon::fromTheme("loadermods"); }
     virtual QString id() const override { return "mods"; }
     virtual QString helpPage() const override { return "Loader-mods"; }

     virtual bool shouldDisplay() const override;

    public slots:
     void updateFrame(const QModelIndex& current, const QModelIndex& previous) override;

    private slots:
     void removeItems(const QItemSelection& selection) override;

     void downloadMods();
     void downloadDialogFinished(int result);
     void updateMods(bool includeDeps = false);
     void deleteModMetadata();
     void exportModMetadata();
     void changeModVersion();
     void onAddProfileClicked();
     void onRemoveProfileClicked();
     void saveCurrentProfileState(bool refuseSuspiciousEmptyOverwrite = false);
     void restoreCurrentProfileToFilesystem();

     void onTabContextMenuRequested(const QPoint& pos);
     void onTabNewToRight(int sourceIndex);
     void onTabDuplicate(int sourceIndex);
     void onTabRename(int tabIndex);
     void onTabRemove(int tabIndex);
     void onTabEnableAll(int tabIndex);
     void onTabDisableAll(int tabIndex);
    private:
     void applyProfileSwitch(int index, int generation, bool isInitialLoad = false);
     void createProfile(const QString& name, const QSet<QString>& initialState, int insertAfterIndex = -1);
     void saveProfileList();
     QString profileListKey() const { return ModProfileKeys::profileListKey(m_settingsPrefix); }
     QString profileKey(const QString& name) const { return ModProfileKeys::profileKey(m_settingsPrefix, name); }
     QString lastActiveIndexKey() const { return ModProfileKeys::lastActiveIndexKey(m_settingsPrefix); }
     QString runtimeProfilesKey() const { return ModProfileKeys::runtimeProfilesKey(m_settingsPrefix); }
     void rebuildRuntimeMenu();
     void onRuntimeProfileToggled(const QString& name, bool checked);
     QStringList loadRuntimeSelection() const;
     void saveRuntimeSelection(const QStringList& selected);
    protected:
     bool eventFilter(QObject* obj, QEvent* ev) override;
     void mousePressEvent(QMouseEvent* event) override;
     void showEvent(QShowEvent* event) override;
     void hideEvent(QHideEvent* event) override;
     ModFolderModel* m_model;
     QPointer<ResourceDownload::ResourceDownloadDialog> m_downloadDialog;
     QPointer<QWidget> m_filterWindow;
     bool m_downloadFlowActive = false;
     QTabBar*       m_profileTabBar  = nullptr;
     QToolButton*   m_newTabButton   = nullptr;
     QToolButton*   m_runtimeProfilesButton = nullptr;
     QMenu*         m_runtimeMenu    = nullptr;
     QMap<QString, QSet<QString>> m_profileStates;
     QString m_currentProfile;
     QString m_settingsPrefix;
     bool m_destructorStarted = false;
     int            m_profileSwitchGeneration = 0;
     bool           m_applyingProfile = false;
     bool           m_instanceRunning = false;
     int            m_inFlightTabIndex = 0;
 };

class CoreModFolderPage : public ModFolderPage {
    Q_OBJECT
   public:
    explicit CoreModFolderPage(MinecraftInstance* inst, ModFolderModel* mods, QWidget* parent = 0);
    virtual ~CoreModFolderPage() = default;

    virtual QString displayName() const override { return tr("Core Mods"); }
    virtual QIcon icon() const override { return QIcon::fromTheme("coremods"); }
    virtual QString id() const override { return "coremods"; }
    virtual QString helpPage() const override { return "Core-mods"; }

    virtual bool shouldDisplay() const override;
};

class NilModFolderPage : public ModFolderPage {
    Q_OBJECT
   public:
    explicit NilModFolderPage(MinecraftInstance* inst, ModFolderModel* mods, QWidget* parent = 0);
    virtual ~NilModFolderPage() = default;

    virtual QString displayName() const override { return tr("Nilmods"); }
    virtual QIcon icon() const override { return QIcon::fromTheme("coremods"); }
    virtual QString id() const override { return "nilmods"; }
    virtual QString helpPage() const override { return "Nilmods"; }

    virtual bool shouldDisplay() const override;
};