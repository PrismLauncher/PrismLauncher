// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <QAbstractListModel>
#include <QDir>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QString>
#include <memory>

#include "Mod.h"
#include "ResourceFolderModel.h"
#include "minecraft/Component.h"
#include "minecraft/mod/Resource.h"

class BaseInstance;
class QFileSystemWatcher;
class VirtualModGroupStore;

/**
 * A legacy mod list.
 * Backed by a folder.
 */
class ModFolderModel : public ResourceFolderModel {
    Q_OBJECT
   public:
    enum Columns {
        ActiveColumn = 0,
        ImageColumn,
        NameColumn,
        VersionColumn,
        DateColumn,
        ProviderColumn,
        SizeColumn,
        SideColumn,
        LoadersColumn,
        McVersionsColumn,
        ReleaseTypeColumn,
        RequiresColumn,
        RequiredByColumn,
        NUM_COLUMNS
    };
    ModFolderModel(const QDir& dir, BaseInstance* instance, bool is_indexed, bool create_dir, QObject* parent = nullptr);
    ~ModFolderModel() override;

    virtual QString id() const override { return "mods"; }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QSortFilterProxyModel* createFilterProxyModel(QObject* parent = nullptr) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex& parent) const override;

    [[nodiscard]] Resource* createResource(const QFileInfo& file) override { return new Mod(file); }
    [[nodiscard]] Task* createParseTask(Resource&) override;

    bool isValid();

    bool setResourceEnabled(const QModelIndexList& indexes, EnableAction action) override;
    bool deleteResources(const QModelIndexList& indexes) override;

    bool createGroup(const QString& name, const QString& parentId = {});
    bool renameGroup(const QString& groupId, const QString& newName);
    bool moveGroup(const QString& groupId, const QString& newParentId);
    bool deleteGroup(const QString& groupId);
    bool assignModsToGroup(const QStringList& fileKeys, const QString& groupId);
    bool setActiveGroup(const QString& groupIdOrAll);
    QList<Resource*> modsForActiveGroupSelection();

    struct GroupOption {
        QString id;
        QString label;
        int depth = 0;
        bool managedPack = false;
    };
    QList<GroupOption> groupOptions() const;
    QString activeGroup() const { return m_activeGroupId; }
    QStringList fileKeysForIndexes(const QModelIndexList& indexes) const;
    QString groupForFileKey(const QString& fileKey) const;
    bool isManagedGroup(const QString& groupId) const;
    bool virtualGroupsEnabled() const { return m_virtualGroupsEnabled; }

    void syncVirtualEntry(Resource* resource);
    void updateManagedPackOwnership(const QStringList& fileNames,
                                    const QString& managedPackType,
                                    const QString& managedPackId,
                                    const QString& managedPackName);
    bool shouldTreatFileAsManagedPackOwned(const QString& fileName, const QString& managedPackType, const QString& managedPackId) const;

    QModelIndexList getAffectedMods(const QModelIndexList& indexes, EnableAction action);

    RESOURCE_HELPERS(Mod)

   public:
    QStringList requiresList(QString id);
    QStringList requiredByList(QString id);

   signals:
    void virtualGroupsChanged();
    void activeGroupChanged(const QString& activeGroupId);

   protected:
    void onUpdateSucceeded() override;

   private slots:
    void onParseSucceeded(int ticket, QString resource_id) override;
    void onParseFinished();

   private:
    class ProxyModel : public ResourceFolderModel::ProxyModel {
       public:
        explicit ProxyModel(QObject* parent = nullptr) : ResourceFolderModel::ProxyModel(parent) {}

       protected:
        bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    };

    void initializeVirtualGroups();
    void migrateLegacyNestedFolders();
    void bootstrapVirtualGroupsFromCurrentState();
    void classifyManagedPackEntriesFromManifests();
    void syncVirtualGroupsFromResources();
    bool isIncompatibleWithInstanceVersion(const Mod& mod) const;
    bool isResourceInActiveGroup(const Resource& resource) const;
    QString fileKeyForResource(const Resource& resource) const;

    QHash<QString, QSet<Mod*>> m_requiredBy;
    QHash<QString, QSet<Mod*>> m_requires;
    QString m_instanceMinecraftVersion;
    bool m_virtualGroupsEnabled = false;
    QString m_activeGroupId;
    std::unique_ptr<VirtualModGroupStore> m_groupStore;
};
