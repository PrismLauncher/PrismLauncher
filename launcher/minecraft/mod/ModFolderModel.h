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
#include <QString>

#include <memory>
#include <vector>

#include "Mod.h"
#include "ResourceFolderModel.h"
#include "minecraft/Component.h"
#include "minecraft/mod/Resource.h"

class BaseInstance;
class QFileSystemWatcher;
class ModGroupStore;

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

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    [[nodiscard]] Resource* createResource(const QFileInfo& file) override { return new Mod(file); }
    [[nodiscard]] Task* createParseTask(Resource&) override;

    bool isValid();

    bool setResourceEnabled(const QModelIndexList& indexes, EnableAction action) override;
    bool deleteResources(const QModelIndexList& indexes) override;

    Mod& at(int index) { return *static_cast<Mod*>(m_resources[index].get()); }
    const Mod& at(int index) const { return *static_cast<const Mod*>(m_resources.at(index).get()); }
    QList<Mod*> selectedMods(const QModelIndexList& indexes);
    QList<Mod*> allMods();
    QList<Resource*> selectedResources(const QModelIndexList& indexes);

    QModelIndexList getAffectedMods(const QModelIndexList& indexes, EnableAction action);

    QStringList requiresList(QString id);
    QStringList requiredByList(QString id);

   private slots:
    void onParseSucceeded(int ticket, QString resource_id) override;
    void onParseFailed(int ticket, QString resource_id) override;
    void onUpdateSucceeded() override;
    void onParseFinished();

   private:
    enum class ItemType { GroupNode, ModNode };

    struct TreeNode {
        ItemType type = ItemType::ModNode;
        TreeNode* parent = nullptr;
        int row = -1;
        QString groupId;
        QString label;
        QString resourceId;
        QList<TreeNode*> children;
    };

    [[nodiscard]] TreeNode* nodeFromIndex(const QModelIndex& index) const;
    [[nodiscard]] Mod* modFromIndex(const QModelIndex& index) const;
    [[nodiscard]] QModelIndex indexForNode(TreeNode* node, int column = 0) const;
    [[nodiscard]] QModelIndex indexForResource(const QString& resourceId, int column = 0) const;

    void rebuildTree();
    void syncGroupAssignments();
    bool setResourcesEnabled(const QList<Mod*>& mods, EnableAction action);

    QHash<QString, QSet<Mod*>> m_requiredBy;
    QHash<QString, QSet<Mod*>> m_requires;

    std::unique_ptr<ModGroupStore> m_groupStore;
    QList<TreeNode*> m_rootNodes;
    std::vector<std::unique_ptr<TreeNode>> m_treeStorage;
    QHash<QString, TreeNode*> m_groupNodesById;
    QHash<QString, TreeNode*> m_resourceNodes;
};
