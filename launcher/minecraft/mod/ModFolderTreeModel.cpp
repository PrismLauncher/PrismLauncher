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

#include "ModFolderTreeModel.h"

#include <QIcon>
#include <QMimeData>
#include <QSet>
#include <QSize>
#include <algorithm>

ModFolderTreeModel::ModFolderTreeModel(ModFolderModel* backend, QObject* parent) : QAbstractItemModel(parent), m_backend(backend)
{
    rebuildTree();
    connect(m_backend, &ResourceFolderModel::updateFinished, this, &ModFolderTreeModel::rebuildTree);
    connect(m_backend, &QAbstractItemModel::dataChanged, this, &ModFolderTreeModel::onBackendDataChanged);
    connect(m_backend, &QAbstractItemModel::modelReset, this, &ModFolderTreeModel::rebuildTree);
}

QModelIndex ModFolderTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    auto* parentNode = nodeForIndex(parent);
    if (!parentNode || row < 0 || row >= static_cast<int>(parentNode->children.size()) || column < 0 || column >= columnCount({})) {
        return {};
    }

    return createIndex(row, column, parentNode->children.at(row).get());
}

QModelIndex ModFolderTreeModel::parent(const QModelIndex& index) const
{
    auto* node = nodeForIndex(index);
    if (!node || node == m_root.get() || !node->parent) {
        return {};
    }

    if (node->parent == m_root.get()) {
        return {};
    }

    return createIndex(node->parent->row, 0, node->parent);
}

int ModFolderTreeModel::rowCount(const QModelIndex& parent) const
{
    auto* node = nodeForIndex(parent);
    return node ? static_cast<int>(node->children.size()) : 0;
}

int ModFolderTreeModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? m_backend->columnCount({}) : m_backend->columnCount({});
}

QVariant ModFolderTreeModel::data(const QModelIndex& index, int role) const
{
    auto* node = nodeForIndex(index);
    if (!node || node == m_root.get()) {
        return {};
    }

    if (node->kind == Node::Kind::MOD && node->backendRow >= 0) {
        auto backendIndex = m_backend->index(node->backendRow, index.column());
        return m_backend->data(backendIndex, role);
    }

    if (node->kind == Node::Kind::FOLDER) {
        switch (role) {
            case Qt::DisplayRole:
                if (index.column() == ModFolderModel::NameColumn) {
                    return node->name;
                }
                if (index.column() == ModFolderModel::VersionColumn) {
                    return tr("Folder");
                }
                return {};
            case Qt::DecorationRole:
                if (index.column() == ModFolderModel::ImageColumn) {
                    return QIcon::fromTheme("folder");
                }
                return {};
            case Qt::SizeHintRole:
                if (index.column() == ModFolderModel::ImageColumn) {
                    return QSize(32, 32);
                }
                return {};
            case Qt::ToolTipRole:
                if (index.column() == ModFolderModel::NameColumn) {
                    return node->dir.absolutePath();
                }
                return {};
            case Qt::CheckStateRole:
                return {};
            default:
                return {};
        }
    }

    return {};
}

bool ModFolderTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    auto* node = nodeForIndex(index);
    if (!node || node->kind != Node::Kind::MOD || node->backendRow < 0) {
        return false;
    }

    auto backendIndex = m_backend->index(node->backendRow, index.column());
    return m_backend->setData(backendIndex, value, role);
}

Qt::ItemFlags ModFolderTreeModel::flags(const QModelIndex& index) const
{
    auto* node = nodeForIndex(index);
    if (!node || node == m_root.get()) {
        return Qt::NoItemFlags;
    }

    if (node->kind == Node::Kind::MOD && node->backendRow >= 0) {
        auto backendIndex = m_backend->index(node->backendRow, index.column());
        return m_backend->flags(backendIndex);
    }

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
}

QVariant ModFolderTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return m_backend->headerData(section, orientation, role);
}

Qt::DropActions ModFolderTreeModel::supportedDropActions() const
{
    return m_backend->supportedDropActions();
}

QStringList ModFolderTreeModel::mimeTypes() const
{
    return m_backend->mimeTypes();
}

bool ModFolderTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    return m_backend->dropMimeData(data, action, row, column, parent);
}

bool ModFolderTreeModel::isFolderIndex(const QModelIndex& index) const
{
    auto* node = nodeForIndex(index);
    return node && node != m_root.get() && node->kind == Node::Kind::FOLDER;
}

Resource* ModFolderTreeModel::resourceForIndex(const QModelIndex& index) const
{
    auto* node = nodeForIndex(index);
    if (!node || node->kind != Node::Kind::MOD) {
        return nullptr;
    }
    return node->resource;
}

Mod* ModFolderTreeModel::modForIndex(const QModelIndex& index) const
{
    return qobject_cast<Mod*>(resourceForIndex(index));
}

QDir ModFolderTreeModel::folderForIndex(const QModelIndex& index) const
{
    auto* node = nodeForIndex(index);
    if (!node || node == m_root.get()) {
        return m_backend->dir();
    }
    if (node->kind == Node::Kind::FOLDER) {
        return node->dir;
    }
    if (node->resource) {
        return QDir(node->resource->fileinfo().absolutePath());
    }
    return m_backend->dir();
}

QList<Resource*> ModFolderTreeModel::resourcesFromIndexes(const QModelIndexList& indexes) const
{
    QList<Resource*> resources;
    QSet<QString> seen;
    for (const auto& index : indexes) {
        if (index.column() != 0) {
            continue;
        }
        auto* resource = resourceForIndex(index);
        if (!resource) {
            continue;
        }
        if (seen.contains(resource->internal_id())) {
            continue;
        }
        seen.insert(resource->internal_id());
        resources.append(resource);
    }
    return resources;
}

QList<Mod*> ModFolderTreeModel::modsFromIndexes(const QModelIndexList& indexes) const
{
    QList<Mod*> mods;
    QSet<QString> seen;
    for (const auto& index : indexes) {
        if (index.column() != 0) {
            continue;
        }
        auto* mod = modForIndex(index);
        if (!mod) {
            continue;
        }
        if (mod->type() == ResourceType::FOLDER || mod->fileinfo().isDir()) {
            continue;
        }
        if (seen.contains(mod->internal_id())) {
            continue;
        }
        seen.insert(mod->internal_id());
        mods.append(mod);
    }
    return mods;
}

QDir ModFolderTreeModel::targetDirForSelection(const QModelIndexList& indexes) const
{
    QSet<QString> folderPaths;
    QDir modDir;
    bool hasModDir = false;

    for (const auto& index : indexes) {
        if (index.column() != 0) {
            continue;
        }
        auto* node = nodeForIndex(index);
        if (!node || node == m_root.get()) {
            continue;
        }
        if (node->kind == Node::Kind::FOLDER) {
            folderPaths.insert(node->dir.absolutePath());
        } else if (node->resource) {
            modDir = QDir(node->resource->fileinfo().absolutePath());
            hasModDir = true;
        }
    }

    if (folderPaths.size() == 1) {
        return QDir(*folderPaths.begin());
    }
    if (folderPaths.isEmpty() && hasModDir) {
        return modDir;
    }
    return m_backend->dir();
}

void ModFolderTreeModel::rebuildTree()
{
    beginResetModel();

    m_root = std::make_unique<Node>();
    m_root->kind = Node::Kind::FOLDER;
    m_root->dir = m_backend->dir();
    m_root->name = m_backend->dir().dirName();
    m_root->parent = nullptr;
    m_root->row = 0;

    m_modNodes.clear();
    QHash<QString, Node*> folderMap;
    folderMap.insert(QString(), m_root.get());

    auto ensureFolderPath = [this, &folderMap](const QString& relPath) {
        if (relPath.isEmpty()) {
            return;
        }

        auto parts = relPath.split('/', Qt::SkipEmptyParts);
        Node* parentNode = m_root.get();
        QString pathAccum;

        for (const auto& part : parts) {
            if (part.startsWith('.')) {
                return;
            }
            pathAccum = pathAccum.isEmpty() ? part : pathAccum + "/" + part;
            if (!folderMap.contains(pathAccum)) {
                auto folderNode = std::make_unique<Node>();
                folderNode->kind = Node::Kind::FOLDER;
                folderNode->name = part;
                folderNode->dir = QDir(m_backend->dir().filePath(pathAccum));
                folderNode->parent = parentNode;

                folderMap.insert(pathAccum, folderNode.get());
                parentNode->children.push_back(std::move(folderNode));
            }
            parentNode = folderMap[pathAccum];
        }
    };

    auto collectFolders = [&](const auto& self, const QDir& dir, const QString& relPrefix) -> void {
        auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& entry : entries) {
            auto name = entry.fileName();
            if (name.startsWith('.')) {
                continue;
            }
            auto relPath = relPrefix.isEmpty() ? name : relPrefix + "/" + name;
            ensureFolderPath(relPath);
            self(self, QDir(entry.absoluteFilePath()), relPath);
        }
    };

    collectFolders(collectFolders, m_backend->dir(), QString());

    auto mods = m_backend->allMods();
    for (int row = 0; row < mods.size(); ++row) {
        auto* mod = mods.at(row);
        if (!mod) {
            continue;
        }
        if (mod->type() == ResourceType::FOLDER || mod->fileinfo().isDir()) {
            continue;
        }

        auto relPath = m_backend->dir().relativeFilePath(mod->fileinfo().absoluteFilePath());
        relPath = QDir::cleanPath(QDir::fromNativeSeparators(relPath));
        if (relPath.startsWith("..")) {
            relPath = mod->fileinfo().fileName();
        }

        auto parts = relPath.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            continue;
        }

        auto fileName = parts.takeLast();
        Node* parentNode = m_root.get();
        QString pathAccum;

        for (const auto& part : parts) {
            pathAccum = pathAccum.isEmpty() ? part : pathAccum + "/" + part;
            ensureFolderPath(pathAccum);
            parentNode = folderMap[pathAccum];
        }

        auto modNode = std::make_unique<Node>();
        modNode->kind = Node::Kind::MOD;
        modNode->name = fileName;
        modNode->resource = mod;
        modNode->backendRow = row;
        modNode->parent = parentNode;

        m_modNodes.insert(mod->internal_id(), modNode.get());
        parentNode->children.push_back(std::move(modNode));
    }

    sortChildren(m_root.get());
    endResetModel();
}

void ModFolderTreeModel::onBackendDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles)
{
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        const auto& resource = m_backend->at(row);
        Node* node = nullptr;
        auto it = m_modNodes.find(resource.internal_id());
        if (it != m_modNodes.end()) {
            node = it.value();
        } else {
            for (auto mapIt = m_modNodes.begin(); mapIt != m_modNodes.end(); ++mapIt) {
                if (mapIt.value()->resource == &resource) {
                    node = mapIt.value();
                    m_modNodes.erase(mapIt);
                    m_modNodes.insert(resource.internal_id(), node);
                    break;
                }
            }
        }
        if (!node) {
            continue;
        }
        auto leftIndex = indexForNode(node, topLeft.column());
        auto rightIndex = indexForNode(node, bottomRight.column());
        if (leftIndex.isValid() && rightIndex.isValid()) {
            emit dataChanged(leftIndex, rightIndex, roles);
        }
    }
}

ModFolderTreeModel::Node* ModFolderTreeModel::nodeForIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return m_root.get();
    }
    return static_cast<Node*>(index.internalPointer());
}

QModelIndex ModFolderTreeModel::indexForNode(Node* node, int column) const
{
    if (!node || node == m_root.get()) {
        return {};
    }
    return createIndex(node->row, column, node);
}

void ModFolderTreeModel::sortChildren(Node* node)
{
    if (!node) {
        return;
    }

    std::sort(node->children.begin(), node->children.end(), [this](const auto& left, const auto& right) {
        if (left->kind != right->kind) {
            return left->kind == Node::Kind::FOLDER;
        }
        return QString::compare(nodeDisplayName(left.get()), nodeDisplayName(right.get()), Qt::CaseInsensitive) < 0;
    });

    for (int i = 0; i < static_cast<int>(node->children.size()); ++i) {
        auto* child = node->children[i].get();
        child->row = i;
        child->parent = node;
        if (child->kind == Node::Kind::FOLDER) {
            sortChildren(child);
        }
    }
}

QString ModFolderTreeModel::nodeDisplayName(const Node* node) const
{
    if (!node) {
        return {};
    }
    if (node->kind == Node::Kind::FOLDER) {
        return node->name;
    }
    if (node->resource) {
        return node->resource->name();
    }
    return node->name;
}

bool ModFolderTreeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    auto* model = qobject_cast<ModFolderTreeModel*>(sourceModel());
    if (!model) {
        return false;
    }

    if (filterRegularExpression().pattern().isEmpty()) {
        return true;
    }

    auto nameIndex = model->index(sourceRow, ModFolderModel::NameColumn, sourceParent);
    if (!nameIndex.isValid()) {
        return false;
    }

    if (model->isFolderIndex(nameIndex)) {
        if (model->data(nameIndex, Qt::DisplayRole).toString().contains(filterRegularExpression())) {
            return true;
        }
        for (int row = 0; row < model->rowCount(nameIndex); ++row) {
            if (filterAcceptsRow(row, nameIndex)) {
                return true;
            }
        }
        return false;
    }

    if (auto* resource = model->resourceForIndex(nameIndex)) {
        return resource->applyFilter(filterRegularExpression());
    }

    return false;
}
