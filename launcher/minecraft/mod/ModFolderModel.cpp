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

#include "ModFolderModel.h"

#include <FileSystem.h>
#include <QAbstractButton>
#include <QDebug>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QIcon>
#include <QMimeData>
#include <QString>
#include <QStyle>
#include <QThreadPool>
#include <QUrl>
#include <algorithm>

#include "minecraft/Component.h"
#include "minecraft/mod/ModGroupStore.h"
#include "minecraft/mod/Resource.h"
#include "minecraft/mod/ResourceFolderModel.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "minecraft/mod/tasks/ResourceFolderLoadTask.h"
#include "modplatform/ModIndex.h"
#include "ui/dialogs/CustomMessageBox.h"

ModFolderModel::ModFolderModel(const QDir& dir, BaseInstance* instance, bool is_indexed, bool create_dir, QObject* parent)
    : ResourceFolderModel(QDir(dir), instance, is_indexed, create_dir, parent), m_groupStore(std::make_unique<ModGroupStore>(m_dir))
{
    m_column_names = QStringList({ "Enable", "Image", "Name", "Version", "Last Modified", "Provider", "Size", "Side", "Loaders",
                                   "Minecraft Versions", "Release Type", "Requires", "Required By" });
    m_column_names_translated =
        QStringList({ tr("Enable"), tr("Image"), tr("Name"), tr("Version"), tr("Last Modified"), tr("Provider"), tr("Size"), tr("Side"),
                      tr("Loaders"), tr("Minecraft Versions"), tr("Release Type"), tr("Requires"), tr("Required By") });
    m_column_sort_keys = { SortType::ENABLED,      SortType::NAME,     SortType::NAME,       SortType::VERSION, SortType::DATE,
                           SortType::PROVIDER,     SortType::SIZE,     SortType::SIDE,       SortType::LOADERS, SortType::MC_VERSIONS,
                           SortType::RELEASE_TYPE, SortType::REQUIRES, SortType::REQUIRED_BY };
    m_column_resize_modes = { QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Stretch,     QHeaderView::Interactive,
                              QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive,
                              QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive,
                              QHeaderView::Interactive };
    m_columnsHideable = { false, true, false, true, true, true, true, true, true, true, true, true, true };

    connect(this, &ModFolderModel::parseFinished, this, &ModFolderModel::onParseFinished);
}

ModFolderModel::~ModFolderModel() = default;

QVariant ModFolderModel::data(const QModelIndex& index, int role) const
{
    auto* node = nodeFromIndex(index);
    if (!node || index.column() < 0 || index.column() >= NUM_COLUMNS)
        return {};

    if (node->type == ItemType::GroupNode) {
        switch (role) {
            case Qt::DisplayRole:
                if (index.column() == NameColumn) {
                    return node->label;
                }
                return {};
            case Qt::ToolTipRole:
                if (index.column() == NameColumn) {
                    return node->label;
                }
                return {};
            default:
                return {};
        }
    }

    auto resourceIndex = m_resources_index.constFind(node->resourceId);
    if (resourceIndex == m_resources_index.constEnd())
        return {};

    int row = resourceIndex.value();
    int column = index.column();

    switch (role) {
        case Qt::BackgroundRole:
            return rowBackground(row);
        case Qt::DisplayRole:
            switch (column) {
                case VersionColumn: {
                    switch (at(row).type()) {
                        case ResourceType::FOLDER:
                            return tr("Folder");
                        case ResourceType::SINGLEFILE:
                            return tr("File");
                        default:
                            return at(row).version();
                    }
                }
                case SideColumn: {
                    return at(row).side();
                }
                case LoadersColumn: {
                    return at(row).loaders();
                }
                case McVersionsColumn: {
                    return at(row).mcVersions();
                }
                case ReleaseTypeColumn: {
                    return at(row).releaseType();
                }
                case RequiredByColumn: {
                    return at(row).requiredByCount();
                }
                case RequiresColumn: {
                    return at(row).requiresCount();
                }
            }
            break;
        case Qt::DecorationRole: {
            if (column == ImageColumn) {
                return at(row).icon({ 32, 32 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding);
            }
            break;
        }
        case Qt::SizeHintRole:
            if (column == ImageColumn) {
                return QSize(32, 32);
            }
            break;
        default:
            break;
    }

    // Delegate common column behavior to the base model, but keep the row mapped
    // to the flat resource index because this model is tree-shaped.
    int mappedColumn = -1;
    switch (column) {
        case ActiveColumn:
            mappedColumn = ResourceFolderModel::ActiveColumn;
            break;
        case NameColumn:
            mappedColumn = ResourceFolderModel::NameColumn;
            break;
        case DateColumn:
            mappedColumn = ResourceFolderModel::DateColumn;
            break;
        case ProviderColumn:
            mappedColumn = ResourceFolderModel::ProviderColumn;
            break;
        case SizeColumn:
            mappedColumn = ResourceFolderModel::SizeColumn;
            break;
    }

    if (mappedColumn >= 0) {
        auto mappedIndex = createIndex(row, mappedColumn, node);
        return ResourceFolderModel::data(mappedIndex, role);
    }

    return {};
}

QModelIndex ModFolderModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || column < 0 || column >= NUM_COLUMNS) {
        return {};
    }

    if (!parent.isValid()) {
        if (row >= m_rootNodes.size()) {
            return {};
        }
        return createIndex(row, column, m_rootNodes.at(row));
    }

    auto* parentNode = nodeFromIndex(parent);
    if (!parentNode || parentNode->type != ItemType::GroupNode || row >= parentNode->children.size()) {
        return {};
    }

    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex ModFolderModel::parent(const QModelIndex& child) const
{
    auto* node = nodeFromIndex(child);
    if (!node || !node->parent) {
        return {};
    }

    return createIndex(node->parent->row, 0, node->parent);
}

int ModFolderModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return m_rootNodes.size();
    }

    if (parent.column() != 0) {
        return 0;
    }

    auto* parentNode = nodeFromIndex(parent);
    if (!parentNode || parentNode->type != ItemType::GroupNode) {
        return 0;
    }

    return parentNode->children.size();
}

QVariant ModFolderModel::headerData(int section, [[maybe_unused]] Qt::Orientation orientation, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case ActiveColumn:
                case NameColumn:
                case VersionColumn:
                case DateColumn:
                case ProviderColumn:
                case ImageColumn:
                case SideColumn:
                case LoadersColumn:
                case McVersionsColumn:
                case ReleaseTypeColumn:
                case SizeColumn:
                case RequiredByColumn:
                case RequiresColumn:
                    return columnNames().at(section);
                default:
                    return QVariant();
            }

        case Qt::ToolTipRole:
            switch (section) {
                case ActiveColumn:
                    return tr("Is the mod enabled?");
                case NameColumn:
                    return tr("The name of the mod.");
                case VersionColumn:
                    return tr("The version of the mod.");
                case DateColumn:
                    return tr("The date and time this mod was last changed (or added).");
                case ProviderColumn:
                    return tr("The source provider of the mod.");
                case SideColumn:
                    return tr("On what environment the mod is running.");
                case LoadersColumn:
                    return tr("The mod loader.");
                case McVersionsColumn:
                    return tr("The supported minecraft versions.");
                case ReleaseTypeColumn:
                    return tr("The release type.");
                case SizeColumn:
                    return tr("The size of the mod.");
                case RequiredByColumn:
                    return tr("For each mod, the number of other mods which depend on it.");
                case RequiresColumn:
                    return tr("For each mod, the number of other mods it depends on.");
                default:
                    return QVariant();
            }
        default:
            return QVariant();
    }
    return QVariant();
}

int ModFolderModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid() && parent.column() != 0) {
        return 0;
    }
    return NUM_COLUMNS;
}

Qt::ItemFlags ModFolderModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;
    }

    auto flags = QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;

    auto* node = nodeFromIndex(index);
    if (!node || node->type == ItemType::GroupNode) {
        return flags & ~Qt::ItemIsUserCheckable;
    }

    if (index.column() == ActiveColumn) {
        flags |= Qt::ItemIsUserCheckable;
    } else {
        flags &= ~Qt::ItemIsUserCheckable;
    }

    return flags;
}

Task* ModFolderModel::createParseTask(Resource& resource)
{
    return new LocalModParseTask(m_next_resolution_ticket, resource.type(), resource.fileinfo());
}

bool ModFolderModel::isValid()
{
    return m_dir.exists() && m_dir.isReadable();
}

QList<Mod*> ModFolderModel::selectedMods(const QModelIndexList& indexes)
{
    QSet<QString> seen;
    QList<Mod*> result;

    for (const auto& index : indexes) {
        if (index.column() != 0) {
            continue;
        }

        auto* mod = modFromIndex(index);
        if (!mod || seen.contains(mod->internal_id())) {
            continue;
        }

        seen.insert(mod->internal_id());
        result.append(mod);
    }

    return result;
}

QList<Mod*> ModFolderModel::allMods()
{
    QList<Mod*> result;
    result.reserve(m_resources.size());

    for (const auto& resource : m_resources) {
        result.append(static_cast<Mod*>(resource.get()));
    }

    return result;
}

QList<Resource*> ModFolderModel::selectedResources(const QModelIndexList& indexes)
{
    QList<Resource*> result;
    for (auto* mod : selectedMods(indexes)) {
        result.append(mod);
    }
    return result;
}

ModFolderModel::TreeNode* ModFolderModel::nodeFromIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    return static_cast<TreeNode*>(index.internalPointer());
}

Mod* ModFolderModel::modFromIndex(const QModelIndex& index) const
{
    auto* node = nodeFromIndex(index);
    if (!node || node->type != ItemType::ModNode) {
        return nullptr;
    }

    auto it = m_resources_index.constFind(node->resourceId);
    if (it == m_resources_index.constEnd()) {
        return nullptr;
    }

    return static_cast<Mod*>(m_resources.at(it.value()).get());
}

QModelIndex ModFolderModel::indexForNode(TreeNode* node, int column) const
{
    if (!node || column < 0 || column >= NUM_COLUMNS) {
        return {};
    }

    return createIndex(node->row, column, node);
}

QModelIndex ModFolderModel::indexForResource(const QString& resourceId, int column) const
{
    auto* node = m_resourceNodes.value(resourceId, nullptr);
    return indexForNode(node, column);
}

void ModFolderModel::syncGroupAssignments()
{
    if (!m_groupStore) {
        return;
    }

    QStringList fileKeys;
    fileKeys.reserve(m_resources.size());
    for (const auto& resource : m_resources) {
        fileKeys.append(ModGroupStore::normalizeFileKey(resource->fileinfo().fileName()));
    }

    m_groupStore->syncWithFilesystem(fileKeys);
}

void ModFolderModel::rebuildTree()
{
    m_rootNodes.clear();
    m_treeStorage.clear();
    m_groupNodesById.clear();
    m_resourceNodes.clear();

    auto createNode = [this](ItemType type) {
        auto node = std::make_unique<TreeNode>();
        node->type = type;
        auto* ptr = node.get();
        m_treeStorage.emplace_back(std::move(node));
        return ptr;
    };

    if (m_groupStore) {
        for (const auto& group : m_groupStore->groups()) {
            auto* groupNode = createNode(ItemType::GroupNode);
            groupNode->groupId = group.id;
            groupNode->label = group.name;
            groupNode->row = m_rootNodes.size();
            m_rootNodes.append(groupNode);
            m_groupNodesById.insert(group.id, groupNode);
        }
    }

    for (const auto& resource : m_resources) {
        auto* modNode = createNode(ItemType::ModNode);
        modNode->resourceId = resource->internal_id();

        TreeNode* parentNode = nullptr;
        if (m_groupStore) {
            auto fileKey = ModGroupStore::normalizeFileKey(resource->fileinfo().fileName());
            auto groupId = m_groupStore->groupFor(fileKey);
            if (!groupId.isEmpty()) {
                parentNode = m_groupNodesById.value(groupId, nullptr);
            }
        }

        if (parentNode) {
            modNode->parent = parentNode;
            modNode->row = parentNode->children.size();
            parentNode->children.append(modNode);
        } else {
            modNode->row = m_rootNodes.size();
            m_rootNodes.append(modNode);
        }

        m_resourceNodes.insert(modNode->resourceId, modNode);
    }
}

void ModFolderModel::onParseSucceeded(int ticket, QString mod_id)
{
    auto iter = m_active_parse_tasks.constFind(ticket);
    if (iter == m_active_parse_tasks.constEnd())
        return;
    if (!m_resources_index.contains(mod_id))
        return;

    auto parse_task = *iter;
    auto cast_task = static_cast<LocalModParseTask*>(parse_task.get());

    Q_ASSERT(cast_task->token() == ticket);

    auto resource = find(mod_id);

    auto result = cast_task->result();
    if (result && resource) {
        auto* mod = static_cast<Mod*>(resource.get());
        mod->finishResolvingWithDetails(std::move(result->details));
    }

    auto left = indexForResource(mod_id, RequiresColumn);
    auto right = indexForResource(mod_id, RequiredByColumn);
    if (left.isValid() && right.isValid()) {
        emit dataChanged(left, right);
    }
}

void ModFolderModel::onParseFailed(int ticket, QString resource_id)
{
    auto iter = m_active_parse_tasks.constFind(ticket);
    if (iter == m_active_parse_tasks.constEnd() || !m_resources_index.contains(resource_id)) {
        return;
    }

    auto removedIndex = m_resources_index[resource_id];
    auto removedIt = m_resources.begin() + removedIndex;
    if (removedIt == m_resources.end()) {
        return;
    }

    beginResetModel();
    m_resources.erase(removedIt);

    m_resources_index.clear();
    int idx = 0;
    for (const auto& resource : qAsConst(m_resources)) {
        m_resources_index[resource->internal_id()] = idx;
        idx++;
    }

    syncGroupAssignments();
    rebuildTree();
    endResetModel();
}

void ModFolderModel::onUpdateSucceeded()
{
    auto updateResults = static_cast<ResourceFolderLoadTask*>(m_current_update_task.get())->result();
    auto& newResources = updateResults->resources;

    auto currentList = m_resources_index.keys();
    QSet<QString> currentSet(currentList.begin(), currentList.end());

    auto newList = newResources.keys();
    QSet<QString> newSet(newList.begin(), newList.end());
    QSet<QString> keptSet = currentSet;
    keptSet.intersect(newSet);

    if (currentSet == newSet) {
        for (const auto& kept : keptSet) {
            auto rowIt = m_resources_index.constFind(kept);
            Q_ASSERT(rowIt != m_resources_index.constEnd());
            auto row = rowIt.value();

            auto& newResource = newResources[kept];
            const auto& currentResource = m_resources.at(row);

            if (newResource->dateTimeChanged() == currentResource->dateTimeChanged()) {
                continue;
            }

            if (currentResource->isResolving()) {
                auto ticket = currentResource->resolutionTicket();
                if (m_active_parse_tasks.contains(ticket)) {
                    auto task = (*m_active_parse_tasks.find(ticket)).get();
                    task->abort();
                }
            }

            m_resources[row].reset(newResource);
            resolveResource(m_resources.at(row));

            auto left = indexForResource(kept, 0);
            auto right = indexForResource(kept, columnCount(QModelIndex()) - 1);
            if (left.isValid() && right.isValid()) {
                emit dataChanged(left, right);
            }
        }

        syncGroupAssignments();
        return;
    }

    beginResetModel();

    {
        for (const auto& kept : keptSet) {
            auto rowIt = m_resources_index.constFind(kept);
            Q_ASSERT(rowIt != m_resources_index.constEnd());
            auto row = rowIt.value();

            auto& newResource = newResources[kept];
            const auto& currentResource = m_resources.at(row);

            if (newResource->dateTimeChanged() == currentResource->dateTimeChanged()) {
                continue;
            }

            if (currentResource->isResolving()) {
                auto ticket = currentResource->resolutionTicket();
                if (m_active_parse_tasks.contains(ticket)) {
                    auto task = (*m_active_parse_tasks.find(ticket)).get();
                    task->abort();
                }
            }

            m_resources[row].reset(newResource);
            resolveResource(m_resources.at(row));
        }
    }

    {
        QSet<QString> removedSet = currentSet;
        removedSet.subtract(newSet);

        QList<int> removedRows;
        for (const auto& removed : removedSet) {
            removedRows.append(m_resources_index[removed]);
        }

        std::sort(removedRows.begin(), removedRows.end(), std::greater<int>());

        for (auto removedIndex : removedRows) {
            auto removedIt = m_resources.begin() + removedIndex;
            if (removedIt == m_resources.end()) {
                continue;
            }

            if ((*removedIt)->isResolving()) {
                auto ticket = (*removedIt)->resolutionTicket();
                if (m_active_parse_tasks.contains(ticket)) {
                    auto task = (*m_active_parse_tasks.find(ticket)).get();
                    task->abort();
                }
            }

            m_resources.erase(removedIt);
        }
    }

    {
        QSet<QString> addedSet = newSet;
        addedSet.subtract(currentSet);

        for (const auto& added : addedSet) {
            auto resource = newResources[added];
            m_resources.append(resource);
            resolveResource(m_resources.last());
        }
    }

    m_resources_index.clear();
    int idx = 0;
    for (const auto& resource : qAsConst(m_resources)) {
        m_resources_index[resource->internal_id()] = idx;
        idx++;
    }

    syncGroupAssignments();
    rebuildTree();

    endResetModel();
}

Mod* findById(QSet<Mod*> mods, QString modId)
{
    auto found = std::find_if(mods.begin(), mods.end(), [modId](Mod* m) { return m->mod_id() == modId; });
    return found != mods.end() ? *found : nullptr;
}

void ModFolderModel::onParseFinished()
{
    if (hasPendingParseTasks()) {
        return;
    }
    auto modsList = allMods();
    auto mods = QSet(modsList.begin(), modsList.end());

    m_requires.clear();
    m_requiredBy.clear();

    auto findByProjectID = [mods](QVariant modId, ModPlatform::ResourceProvider provider) -> Mod* {
        auto found = std::find_if(mods.begin(), mods.end(), [modId, provider](Mod* m) {
            return m->metadata() && m->metadata()->provider == provider && m->metadata()->project_id == modId;
        });
        return found != mods.end() ? *found : nullptr;
    };
    for (auto mod : mods) {
        auto id = mod->mod_id();
        for (auto dep : mod->dependencies()) {
            auto d = findById(mods, dep);
            if (d) {
                m_requires[id] << d;
                m_requiredBy[d->mod_id()] << mod;
            }
        }
        if (mod->metadata()) {
            for (auto dep : mod->metadata()->dependencies) {
                if (dep.type == ModPlatform::DependencyType::REQUIRED) {
                    auto d = findByProjectID(dep.addonId, mod->metadata()->provider);
                    if (d) {
                        m_requires[id] << d;
                        m_requiredBy[d->mod_id()] << mod;
                    }
                }
            }
        }
    }
    for (auto mod : mods) {
        auto id = mod->mod_id();
        if (mod->requiredByCount() != m_requiredBy[id].count() || mod->requiresCount() != m_requires[id].count()) {
            mod->setRequiredByCount(m_requiredBy[id].count());
            mod->setRequiresCount(m_requires[id].count());
            auto left = indexForResource(mod->internal_id(), 0);
            auto right = indexForResource(mod->internal_id(), columnCount(QModelIndex()) - 1);
            if (left.isValid() && right.isValid()) {
                emit dataChanged(left, right);
            }
        }
    }
}

QSet<Mod*> collectMods(QSet<Mod*> mods, QHash<QString, QSet<Mod*>> relation, std::set<QString>& seen, bool shouldBeEnabled)
{
    QSet<Mod*> affectedList = {};
    QSet<Mod*> needToCheck = {};
    for (auto mod : mods) {
        auto id = mod->mod_id();
        if (seen.count(id) == 0) {
            seen.insert(id);
            for (auto affected : relation[id]) {
                auto affectedId = affected->mod_id();

                if (findById(mods, affectedId) == nullptr && seen.count(affectedId) == 0) {
                    seen.insert(affectedId);
                    if (shouldBeEnabled != affected->enabled()) {
                        affectedList << affected;
                    }
                    needToCheck << affected;
                }
            }
        }
    }
    // collect the affected mods until all of them are included in the list
    if (!needToCheck.isEmpty()) {
        affectedList += collectMods(needToCheck, relation, seen, shouldBeEnabled);
    }
    return affectedList;
}

QModelIndexList ModFolderModel::getAffectedMods(const QModelIndexList& indexes, EnableAction action)
{
    if (indexes.isEmpty())
        return {};

    QModelIndexList affectedList = {};
    auto affectedModsList = selectedMods(indexes);
    auto affectedMods = QSet(affectedModsList.begin(), affectedModsList.end());
    std::set<QString> seen;

    switch (action) {
        case EnableAction::ENABLE: {
            affectedMods = collectMods(affectedMods, m_requires, seen, true);
            break;
        }
        case EnableAction::DISABLE: {
            affectedMods = collectMods(affectedMods, m_requiredBy, seen, false);
            break;
        }
        case EnableAction::TOGGLE: {
            return {};  // this function should not be called with TOGGLE
        }
    }
    for (auto affected : affectedMods) {
        auto affectedIndex = indexForResource(affected->internal_id(), 0);
        if (affectedIndex.isValid()) {
            affectedList << affectedIndex;
        }
    }
    return affectedList;
}

bool ModFolderModel::setResourcesEnabled(const QList<Mod*>& mods, EnableAction action)
{
    bool succeeded = true;

    for (auto* mod : mods) {
        if (!mod) {
            continue;
        }

        auto oldId = mod->internal_id();
        auto rowIt = m_resources_index.constFind(oldId);
        if (rowIt == m_resources_index.constEnd()) {
            continue;
        }

        auto row = rowIt.value();
        auto& resource = m_resources[row];

        if (!resource->enable(action)) {
            succeeded = false;
            continue;
        }

        auto newId = resource->internal_id();
        m_resources_index.remove(oldId);
        m_resources_index.insert(newId, row);

        auto* node = m_resourceNodes.value(oldId, nullptr);
        if (node) {
            m_resourceNodes.remove(oldId);
            node->resourceId = newId;
            m_resourceNodes.insert(newId, node);
        }

        auto left = indexForResource(newId, 0);
        auto right = indexForResource(newId, columnCount(QModelIndex()) - 1);
        if (left.isValid() && right.isValid()) {
            emit dataChanged(left, right);
        }
    }

    return succeeded;
}

bool ModFolderModel::setResourceEnabled(const QModelIndexList& indexes, EnableAction action)
{
    if (indexes.isEmpty())
        return true;

    if (m_instance != nullptr && m_instance->isRunning()) {
        auto response =
            CustomMessageBox::selectable(nullptr, tr("Confirm toggle"),
                                         tr("If you enable/disable this resource while the game is running it may crash your game.\n"
                                            "Are you sure you want to do this?"),
                                         QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                ->exec();

        if (response != QMessageBox::Yes)
            return false;
    }

    auto indexedModsList = selectedMods(indexes);
    auto indexedMods = QSet(indexedModsList.begin(), indexedModsList.end());

    QSet<Mod*> toEnable = {};
    QSet<Mod*> toDisable = {};
    std::set<QString> seen;

    switch (action) {
        case EnableAction::ENABLE: {
            toEnable = indexedMods;
            break;
        }
        case EnableAction::DISABLE: {
            toDisable = indexedMods;
            break;
        }
        case EnableAction::TOGGLE: {
            for (auto mod : indexedMods) {
                if (mod->enabled()) {
                    toDisable << mod;
                } else {
                    toEnable << mod;
                }
            }
            break;
        }
    }

    auto requiredToEnable = collectMods(toEnable, m_requires, seen, true);
    auto requiredToDisable = collectMods(toDisable, m_requiredBy, seen, false);

    toDisable.removeIf([toEnable](Mod* m) { return toEnable.contains(m); });

    if (requiredToEnable.size() > 0 || requiredToDisable.size() > 0) {
        QString title;
        QString message;
        QString noButton;
        QString yesButton;
        if (requiredToEnable.size() > 0 && requiredToDisable.size() > 0) {
            title = tr("Confirm toggle");
            message = tr("Toggling these mod(s) will cause changes to other mods.\n") +
                      tr("%n mod(s) will be enabled\n", "", requiredToEnable.size()) +
                      tr("%n mod(s) will be disabled\n", "", requiredToDisable.size()) +
                      tr("Do you want to automatically apply these related changes?\nIgnoring them may break the game.");
            noButton = tr("Only Toggle Selected");
            yesButton = tr("Toggle Required Mods");
        } else if (requiredToEnable.size() > 0) {
            title = tr("Confirm enable");
            message = tr("The enabled mod(s) require %n mod(s).\n", "", requiredToEnable.size()) +
                      tr("Would you like to enable them as well?\nIgnoring them may break the game.");
            noButton = tr("Only Enable Selected");
            yesButton = tr("Enable Required");
        } else {
            title = tr("Confirm disable");
            message = tr("The disabled mod(s) are required by %n mod(s).\n", "", requiredToDisable.size()) +
                      tr("Would you like to disable them as well?\nIgnoring them may break the game.");
            noButton = tr("Only Disable Selected");
            yesButton = tr("Disable Required");
        }

        auto box = CustomMessageBox::selectable(nullptr, title, message, QMessageBox::Warning,
                                                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
        box->button(QMessageBox::No)->setText(noButton);
        box->button(QMessageBox::Yes)->setText(yesButton);
        auto response = box->exec();

        if (response == QMessageBox::Yes) {
            toEnable |= requiredToEnable;
            toDisable |= requiredToDisable;
        } else if (response == QMessageBox::Cancel) {
            return false;
        }
    }

    auto disableStatus = setResourcesEnabled(toDisable.values(), EnableAction::DISABLE);
    auto enableStatus = setResourcesEnabled(toEnable.values(), EnableAction::ENABLE);
    return disableStatus && enableStatus;
}

QStringList reqToList(QSet<Mod*> l)
{
    QStringList req;
    for (auto m : l) {
        req << m->name();
    }
    return req;
}

QStringList ModFolderModel::requiresList(QString id)
{
    return reqToList(m_requires[id]);
}

QStringList ModFolderModel::requiredByList(QString id)
{
    return reqToList(m_requiredBy[id]);
}

bool ModFolderModel::deleteResources(const QModelIndexList& indexes)
{
    auto deleteInvalid = [](QSet<Mod*>& mods) {
        for (auto it = mods.begin(); it != mods.end();) {
            auto mod = *it;
            // the QFileInfo::exists is used instead of mod->fileinfo().exists
            // because the later somehow caches that the file exists
            if (!mod || !QFileInfo::exists(mod->fileinfo().absoluteFilePath())) {
                it = mods.erase(it);
            } else {
                ++it;
            }
        }
    };
    auto mods = selectedMods(indexes);
    if (mods.isEmpty()) {
        return true;
    }

    for (auto* mod : mods) {
        static_cast<Resource*>(mod)->destroy(indexDir());
    }

    update();
    for (auto mod : allMods()) {
        auto id = mod->mod_id();
        deleteInvalid(m_requiredBy[id]);
        deleteInvalid(m_requires[id]);
        if (mod->requiredByCount() != m_requiredBy[id].count() || mod->requiresCount() != m_requires[id].count()) {
            mod->setRequiredByCount(m_requiredBy[id].count());
            mod->setRequiresCount(m_requires[id].count());
            auto left = indexForResource(mod->internal_id(), RequiresColumn);
            auto right = indexForResource(mod->internal_id(), RequiredByColumn);
            if (left.isValid() && right.isValid()) {
                emit dataChanged(left, right);
            }
        }
    }
    return true;
}
