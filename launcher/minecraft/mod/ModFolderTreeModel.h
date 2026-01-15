// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 abhicommands <114682464+abhicommands@users.noreply.github.com>
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
 */

#pragma once

#include <QAbstractItemModel>
#include <QDir>
#include <QHash>
#include <QSortFilterProxyModel>
#include <memory>
#include <vector>

#include "ModFolderModel.h"

class ModFolderTreeModel : public QAbstractItemModel {
    Q_OBJECT
   public:
    explicit ModFolderTreeModel(ModFolderModel* backend, QObject* parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool isFolderIndex(const QModelIndex& index) const;
    Resource* resourceForIndex(const QModelIndex& index) const;
    Mod* modForIndex(const QModelIndex& index) const;
    QDir folderForIndex(const QModelIndex& index) const;
    QList<Resource*> resourcesFromIndexes(const QModelIndexList& indexes) const;
    QList<Mod*> modsFromIndexes(const QModelIndexList& indexes) const;
    QDir targetDirForSelection(const QModelIndexList& indexes) const;
    bool hasFolderNodes() const;

    QList<Mod*> allMods() const { return m_backend->allMods(); }
    ModFolderModel* backend() const { return m_backend; }

   private slots:
    void rebuildTree();
    void onBackendDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles);

   private:
    struct Node {
        enum class Kind { FOLDER, MOD };
        Kind kind = Kind::FOLDER;
        QString name;
        QDir dir;
        Resource* resource = nullptr;
        int backendRow = -1;
        int row = 0;
        Node* parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;
    };

    Node* nodeForIndex(const QModelIndex& index) const;
    QModelIndex indexForNode(Node* node, int column) const;
    void sortChildren(Node* node);
    QString nodeDisplayName(const Node* node) const;

   private:
    ModFolderModel* m_backend = nullptr;
    std::unique_ptr<Node> m_root;
    QHash<QString, Node*> m_modNodes;
};

class ModFolderTreeProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
   public:
    explicit ModFolderTreeProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};
