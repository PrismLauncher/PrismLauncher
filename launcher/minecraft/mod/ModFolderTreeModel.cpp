// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024
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
    auto* parent_node = nodeForIndex(parent);
    if (!parent_node || row < 0 || row >= static_cast<int>(parent_node->children.size()) || column < 0 || column >= columnCount({}))
        return {};

    return createIndex(row, column, parent_node->children.at(row).get());
}

QModelIndex ModFolderTreeModel::parent(const QModelIndex& index) const
{
    auto* node = nodeForIndex(index);
    if (!node || node == m_root.get() || !node->parent)
        return {};

    if (node->parent == m_root.get())
        return {};

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
    if (!node || node == m_root.get())
        return {};

    if (node->kind == Node::Kind::MOD && node->backend_row >= 0) {
        auto backend_index = m_backend->index(node->backend_row, index.column());
        return m_backend->data(backend_index, role);
    }

    if (node->kind == Node::Kind::FOLDER) {
        switch (role) {
            case Qt::DisplayRole:
                if (index.column() == ModFolderModel::NameColumn)
                    return node->name;
                if (index.column() == ModFolderModel::VersionColumn)
                    return tr("Folder");
                return {};
            case Qt::DecorationRole:
                if (index.column() == ModFolderModel::ImageColumn)
                    return QIcon::fromTheme("folder");
                return {};
            case Qt::SizeHintRole:
                if (index.column() == ModFolderModel::ImageColumn)
                    return QSize(32, 32);
                return {};
            case Qt::ToolTipRole:
                if (index.column() == ModFolderModel::NameColumn)
                    return node->dir.absolutePath();
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
    if (!node || node->kind != Node::Kind::MOD || node->backend_row < 0)
        return false;

    auto backend_index = m_backend->index(node->backend_row, index.column());
    return m_backend->setData(backend_index, value, role);
}

Qt::ItemFlags ModFolderTreeModel::flags(const QModelIndex& index) const
{
    auto* node = nodeForIndex(index);
    if (!node || node == m_root.get())
        return Qt::NoItemFlags;

    if (node->kind == Node::Kind::MOD && node->backend_row >= 0) {
        auto backend_index = m_backend->index(node->backend_row, index.column());
        return m_backend->flags(backend_index);
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
    if (!node || node->kind != Node::Kind::MOD)
        return nullptr;
    return node->resource;
}

Mod* ModFolderTreeModel::modForIndex(const QModelIndex& index) const
{
    return qobject_cast<Mod*>(resourceForIndex(index));
}

QDir ModFolderTreeModel::folderForIndex(const QModelIndex& index) const
{
    auto* node = nodeForIndex(index);
    if (!node || node == m_root.get())
        return m_backend->dir();
    if (node->kind == Node::Kind::FOLDER)
        return node->dir;
    if (node->resource)
        return QDir(node->resource->fileinfo().absolutePath());
    return m_backend->dir();
}

QList<Resource*> ModFolderTreeModel::resourcesFromIndexes(const QModelIndexList& indexes) const
{
    QList<Resource*> resources;
    QSet<QString> seen;
    for (const auto& index : indexes) {
        if (index.column() != 0)
            continue;
        auto* resource = resourceForIndex(index);
        if (!resource)
            continue;
        if (seen.contains(resource->internal_id()))
            continue;
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
        if (index.column() != 0)
            continue;
        auto* mod = modForIndex(index);
        if (!mod)
            continue;
        if (mod->type() == ResourceType::FOLDER || mod->fileinfo().isDir())
            continue;
        if (seen.contains(mod->internal_id()))
            continue;
        seen.insert(mod->internal_id());
        mods.append(mod);
    }
    return mods;
}

QDir ModFolderTreeModel::targetDirForSelection(const QModelIndexList& indexes) const
{
    QSet<QString> folder_paths;
    QDir mod_dir;
    bool has_mod_dir = false;

    for (const auto& index : indexes) {
        if (index.column() != 0)
            continue;
        auto* node = nodeForIndex(index);
        if (!node || node == m_root.get())
            continue;
        if (node->kind == Node::Kind::FOLDER) {
            folder_paths.insert(node->dir.absolutePath());
        } else if (node->resource) {
            mod_dir = QDir(node->resource->fileinfo().absolutePath());
            has_mod_dir = true;
        }
    }

    if (folder_paths.size() == 1)
        return QDir(*folder_paths.begin());
    if (folder_paths.isEmpty() && has_mod_dir)
        return mod_dir;
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
    QHash<QString, Node*> folder_map;
    folder_map.insert(QString(), m_root.get());

    auto ensureFolderPath = [this, &folder_map](const QString& rel_path) {
        if (rel_path.isEmpty())
            return;

        auto parts = rel_path.split('/', Qt::SkipEmptyParts);
        Node* parent_node = m_root.get();
        QString path_accum;

        for (const auto& part : parts) {
            if (part.startsWith('.'))
                return;
            path_accum = path_accum.isEmpty() ? part : path_accum + "/" + part;
            if (!folder_map.contains(path_accum)) {
                auto folder_node = std::make_unique<Node>();
                folder_node->kind = Node::Kind::FOLDER;
                folder_node->name = part;
                folder_node->dir = QDir(m_backend->dir().filePath(path_accum));
                folder_node->parent = parent_node;

                folder_map.insert(path_accum, folder_node.get());
                parent_node->children.push_back(std::move(folder_node));
            }
            parent_node = folder_map[path_accum];
        }
    };

    auto collectFolders = [&](const auto& self, const QDir& dir, const QString& rel_prefix) -> void {
        auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& entry : entries) {
            auto name = entry.fileName();
            if (name.startsWith('.'))
                continue;
            auto rel_path = rel_prefix.isEmpty() ? name : rel_prefix + "/" + name;
            ensureFolderPath(rel_path);
            self(self, QDir(entry.absoluteFilePath()), rel_path);
        }
    };

    collectFolders(collectFolders, m_backend->dir(), QString());

    auto mods = m_backend->allMods();
    for (int row = 0; row < mods.size(); ++row) {
        auto* mod = mods.at(row);
        if (!mod)
            continue;
        if (mod->type() == ResourceType::FOLDER || mod->fileinfo().isDir())
            continue;

        auto rel_path = m_backend->dir().relativeFilePath(mod->fileinfo().absoluteFilePath());
        rel_path = QDir::cleanPath(QDir::fromNativeSeparators(rel_path));
        if (rel_path.startsWith(".."))
            rel_path = mod->fileinfo().fileName();

        auto parts = rel_path.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty())
            continue;

        auto file_name = parts.takeLast();
        Node* parent_node = m_root.get();
        QString path_accum;

        for (const auto& part : parts) {
            path_accum = path_accum.isEmpty() ? part : path_accum + "/" + part;
            ensureFolderPath(path_accum);
            parent_node = folder_map[path_accum];
        }

        auto mod_node = std::make_unique<Node>();
        mod_node->kind = Node::Kind::MOD;
        mod_node->name = file_name;
        mod_node->resource = mod;
        mod_node->backend_row = row;
        mod_node->parent = parent_node;

        m_modNodes.insert(mod->internal_id(), mod_node.get());
        parent_node->children.push_back(std::move(mod_node));
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
            for (auto map_it = m_modNodes.begin(); map_it != m_modNodes.end(); ++map_it) {
                if (map_it.value()->resource == &resource) {
                    node = map_it.value();
                    m_modNodes.erase(map_it);
                    m_modNodes.insert(resource.internal_id(), node);
                    break;
                }
            }
        }
        if (!node)
            continue;
        auto left = indexForNode(node, topLeft.column());
        auto right = indexForNode(node, bottomRight.column());
        if (left.isValid() && right.isValid())
            emit dataChanged(left, right, roles);
    }
}

ModFolderTreeModel::Node* ModFolderTreeModel::nodeForIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return m_root.get();
    return static_cast<Node*>(index.internalPointer());
}

QModelIndex ModFolderTreeModel::indexForNode(Node* node, int column) const
{
    if (!node || node == m_root.get())
        return {};
    return createIndex(node->row, column, node);
}

void ModFolderTreeModel::sortChildren(Node* node)
{
    if (!node)
        return;

    std::sort(node->children.begin(), node->children.end(), [this](const auto& left, const auto& right) {
        if (left->kind != right->kind)
            return left->kind == Node::Kind::FOLDER;
        return QString::compare(nodeDisplayName(left.get()), nodeDisplayName(right.get()), Qt::CaseInsensitive) < 0;
    });

    for (int i = 0; i < static_cast<int>(node->children.size()); ++i) {
        auto* child = node->children[i].get();
        child->row = i;
        child->parent = node;
        if (child->kind == Node::Kind::FOLDER)
            sortChildren(child);
    }
}

QString ModFolderTreeModel::nodeDisplayName(const Node* node) const
{
    if (!node)
        return {};
    if (node->kind == Node::Kind::FOLDER)
        return node->name;
    if (node->resource)
        return node->resource->name();
    return node->name;
}

bool ModFolderTreeProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    auto* model = qobject_cast<ModFolderTreeModel*>(sourceModel());
    if (!model)
        return false;

    if (filterRegularExpression().pattern().isEmpty())
        return true;

    auto name_index = model->index(source_row, ModFolderModel::NameColumn, source_parent);
    if (!name_index.isValid())
        return false;

    if (model->isFolderIndex(name_index)) {
        if (model->data(name_index, Qt::DisplayRole).toString().contains(filterRegularExpression()))
            return true;
        for (int row = 0; row < model->rowCount(name_index); ++row) {
            if (filterAcceptsRow(row, name_index))
                return true;
        }
        return false;
    }

    if (auto* resource = model->resourceForIndex(name_index))
        return resource->applyFilter(filterRegularExpression());

    return false;
}
