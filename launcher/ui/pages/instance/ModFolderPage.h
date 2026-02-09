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
#include <QSet>
#include <QStringList>
#include "ExternalResourcesPage.h"
#include "ui/dialogs/ResourceDownloadDialog.h"

class Mod;
class Resource;
class QAction;
class VirtualModTreeModel;
class VirtualModTreeProxyModel;

class ModFolderPage : public ExternalResourcesPage {
    Q_OBJECT

    inline bool handleNoModLoader();

   public:
    explicit ModFolderPage(BaseInstance* inst, ModFolderModel* model, QWidget* parent = nullptr);
    ~ModFolderPage() override = default;

    void setFilter(const QString& filter) { m_fileSelectionFilter = filter; }

    QString displayName() const override { return tr("Mods"); }
    QIcon icon() const override { return QIcon::fromTheme("loadermods"); }
    QString id() const override { return "mods"; }
    QString helpPage() const override { return "Loader-mods"; }

    bool shouldDisplay() const override;

   public slots:
    void updateActions() override;
    void updateFrame(const QModelIndex& current, const QModelIndex& previous) override;

   private slots:
    void removeItem();
    void removeItems(const QItemSelection& selection) override;
    void itemActivated(const QModelIndex& index);

    void downloadMods();
    void downloadDialogFinished(int result);
    void updateMods(bool includeDeps = false);
    void deleteModMetadata();
    void exportModMetadata();
    void changeModVersion();
    void createGroup();
    void deleteSelectedGroup();
    void moveSelectedModsToGroup();
    void captureTreeStateBeforeReset();
    void restoreTreeStateAfterReset();

   private:
    QModelIndexList selectedSourceIndexes() const;
    QList<Resource*> selectedResources() const;
    QList<Mod*> selectedMods() const;
    QList<Resource*> resourcesForUpdateSelection(const QModelIndexList& sourceIndexes) const;
    QModelIndexList backendIndexesForResources(const QList<Resource*>& resources) const;
    QModelIndexList backendIndexesForGroup(const QString& groupId) const;
    QString selectedGroupIdForActions() const;
    QString currentSelectedGroupId() const;
    QString selectedTargetGroupId() const;
    QString groupDisplayName(const QString& groupId) const;
    bool confirmManagedGroupModification(const QString& groupId, const QString& actionDescription);

   protected:
    void enableItem() override;
    void disableItem() override;
    void viewHomepage() override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

   protected:
    ModFolderModel* m_model;
    QPointer<ResourceDownload::ModDownloadDialog> m_downloadDialog;
    VirtualModTreeModel* m_treeModel = nullptr;
    VirtualModTreeProxyModel* m_treeFilterModel = nullptr;
    QAction* m_actionCreateGroup = nullptr;
    QAction* m_actionDeleteGroup = nullptr;
    QAction* m_actionMoveToGroup = nullptr;
    QSet<QString> m_preDownloadFileKeys;
    QString m_pendingDownloadGroupId;
    QMetaObject::Connection m_downloadUpdateConnection;
    QStringList m_pendingSelectedGroupIds;
    QStringList m_pendingSelectedFileKeys;
    QStringList m_pendingExpandedGroupIds;
    bool m_pendingScrollValid = false;
    int m_pendingVerticalScrollValue = 0;
    int m_pendingHorizontalScrollValue = 0;
};

class CoreModFolderPage : public ModFolderPage {
    Q_OBJECT
   public:
    explicit CoreModFolderPage(BaseInstance* inst, ModFolderModel* mods, QWidget* parent = 0);
    ~CoreModFolderPage() override = default;

    QString displayName() const override { return tr("Core Mods"); }
    QIcon icon() const override { return QIcon::fromTheme("coremods"); }
    QString id() const override { return "coremods"; }
    QString helpPage() const override { return "Core-mods"; }

    bool shouldDisplay() const override;
};

class NilModFolderPage : public ModFolderPage {
    Q_OBJECT
   public:
    explicit NilModFolderPage(BaseInstance* inst, ModFolderModel* mods, QWidget* parent = 0);
    ~NilModFolderPage() override = default;

    QString displayName() const override { return tr("Nilmods"); }
    QIcon icon() const override { return QIcon::fromTheme("coremods"); }
    QString id() const override { return "nilmods"; }
    QString helpPage() const override { return "Nilmods"; }

    bool shouldDisplay() const override;
};
