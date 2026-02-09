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

#include "ModFolderPage.h"
#include "minecraft/mod/Resource.h"
#include "ui/dialogs/ExportToModListDialog.h"
#include "ui/dialogs/InstallLoaderDialog.h"
#include "ui_ExternalResourcesPage.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QEvent>
#include <QHash>
#include <QIcon>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QSet>
#include <QSize>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

#include "Application.h"
#include "DesktopServices.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/dialogs/ResourceUpdateDialog.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/VirtualModGroupStore.h"

#include "tasks/ConcurrentTask.h"
#include "tasks/Task.h"
#include "ui/dialogs/ProgressDialog.h"

constexpr auto MOD_FILE_KEYS_MIME_TYPE = "application/x-prismlauncher-mod-filekeys";

[[nodiscard]] QSet<QString> collectCurrentModFileKeys(const ModFolderModel* model)
{
    QSet<QString> fileKeys;
    if (model == nullptr) {
        return fileKeys;
    }

    for (int row = 0; row < model->rowCount(); ++row) {
        auto const& resource = model->at(row);
        if (resource.type() == ResourceType::FOLDER) {
            continue;
        }

        auto fileKey = VirtualModGroupStore::fileKeyForFileName(resource.getOriginalFileName());
        if (!fileKey.isEmpty()) {
            fileKeys.insert(fileKey);
        }
    }

    return fileKeys;
}

class VirtualModTreeModel : public QAbstractItemModel {
   public:
    explicit VirtualModTreeModel(ModFolderModel* backend, QObject* parent = nullptr) : QAbstractItemModel(parent), m_backend(backend)
    {
        rebuildTree();
        connect(m_backend, &ResourceFolderModel::updateFinished, this, &VirtualModTreeModel::rebuildTree);
        connect(m_backend, &QAbstractItemModel::rowsInserted, this, [this] { rebuildTree(); });
        connect(m_backend, &QAbstractItemModel::rowsRemoved, this, [this] { rebuildTree(); });
        connect(m_backend, &QAbstractItemModel::modelReset, this, [this] { rebuildTree(); });
        connect(m_backend, &QAbstractItemModel::dataChanged, this, &VirtualModTreeModel::onBackendDataChanged);
        connect(m_backend, &ModFolderModel::virtualGroupsChanged, this, [this] { rebuildTree(); });
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override
    {
        auto* parentNode = nodeForIndex(parent);
        if (parentNode == nullptr || row < 0 || std::cmp_greater_equal(row, parentNode->children.size()) || column < 0 ||
            column >= columnCount({})) {
            return {};
        }

        return createIndex(row, column, parentNode->children.at(row).get());
    }

    QModelIndex parent(const QModelIndex& index) const override
    {
        auto* node = nodeForIndex(index);
        if (node == nullptr || node == m_root.get() || node->parent == nullptr) {
            return {};
        }

        if (node->parent == m_root.get()) {
            return {};
        }

        return createIndex(node->parent->row, 0, node->parent);
    }

    int rowCount(const QModelIndex& parent = {}) const override
    {
        auto* node = nodeForIndex(parent);
        return node == nullptr ? 0 : static_cast<int>(node->children.size());
    }

    int columnCount(const QModelIndex& parent = {}) const override
    {
        Q_UNUSED(parent)
        return m_backend->columnCount({});
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        auto* node = nodeForIndex(index);
        if (node == nullptr || node == m_root.get()) {
            return {};
        }

        if (node->kind == Node::Kind::MOD && node->backendRow >= 0) {
            auto backendIndex = m_backend->index(node->backendRow, index.column());
            return m_backend->data(backendIndex, role);
        }

        if (node->kind == Node::Kind::GROUP) {
            switch (role) {
                case Qt::DisplayRole:
                    if (index.column() == ModFolderModel::NameColumn) {
                        return node->name;
                    }
                    if (index.column() == ModFolderModel::VersionColumn) {
                        return QObject::tr("Group");
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
                        return node->groupId.isEmpty() ? QObject::tr("Ungrouped mods") : QObject::tr("Group");
                    }
                    return {};
                default:
                    return {};
            }
        }

        return {};
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        auto* node = nodeForIndex(index);
        if (node == nullptr || node->kind != Node::Kind::MOD || node->backendRow < 0) {
            return false;
        }

        auto backendIndex = m_backend->index(node->backendRow, index.column());
        return m_backend->setData(backendIndex, value, role);
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (!index.isValid()) {
            return Qt::ItemIsDropEnabled;
        }

        auto* node = nodeForIndex(index);
        if (node == nullptr) {
            return Qt::NoItemFlags;
        }

        if (node->kind == Node::Kind::MOD && node->backendRow >= 0) {
            auto backendIndex = m_backend->index(node->backendRow, index.column());
            auto backendFlags = m_backend->flags(backendIndex);
            backendFlags |= Qt::ItemIsDragEnabled;
            backendFlags &= ~Qt::ItemIsDropEnabled;
            return backendFlags;
        }

        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        return m_backend->headerData(section, orientation, role);
    }

    Qt::DropActions supportedDropActions() const override { return m_backend->supportedDropActions(); }

    Qt::DropActions supportedDragActions() const override { return Qt::MoveAction; }

    QStringList mimeTypes() const override
    {
        auto types = m_backend->mimeTypes();
        if (!types.contains(MOD_FILE_KEYS_MIME_TYPE)) {
            types.append(MOD_FILE_KEYS_MIME_TYPE);
        }
        return types;
    }

    QMimeData* mimeData(const QModelIndexList& indexes) const override
    {
        auto* mimeData = new QMimeData();

        QStringList fileKeys;
        QSet<QString> seen;
        for (auto const& index : indexes) {
            if (index.column() != 0) {
                continue;
            }

            auto* resource = resourceForIndex(index);
            if (resource == nullptr || resource->type() == ResourceType::FOLDER) {
                continue;
            }

            auto fileKey = VirtualModGroupStore::fileKeyForFileName(resource->getOriginalFileName());
            if (fileKey.isEmpty() || seen.contains(fileKey)) {
                continue;
            }

            seen.insert(fileKey);
            fileKeys.append(fileKey);
        }

        if (!fileKeys.isEmpty()) {
            mimeData->setData(MOD_FILE_KEYS_MIME_TYPE, fileKeys.join('\n').toUtf8());
        }

        return mimeData;
    }

    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override
    {
        if (action == Qt::IgnoreAction) {
            return true;
        }

        if (data == nullptr || !(action & supportedDropActions())) {
            return false;
        }

        if (data->hasFormat(MOD_FILE_KEYS_MIME_TYPE)) {
            auto payload = QString::fromUtf8(data->data(MOD_FILE_KEYS_MIME_TYPE));
            auto fileKeys = payload.split('\n', Qt::SkipEmptyParts);
            fileKeys.removeDuplicates();
            if (fileKeys.isEmpty()) {
                return false;
            }

            QString targetGroupId;
            auto* targetNode = nodeForIndex(parent);

            if (targetNode == nullptr || targetNode == m_root.get()) {
                if (row >= 0 && row < rowCount({})) {
                    auto targetIndex = index(row, 0, {});
                    targetNode = nodeForIndex(targetIndex);
                }
            }

            if (targetNode != nullptr && targetNode != m_root.get()) {
                targetGroupId = targetNode->groupId;
            }

            if (!targetGroupId.isEmpty() && m_backend->isManagedGroup(targetGroupId)) {
                auto groupName = targetNode != nullptr ? targetNode->name : QObject::tr("selected group");
                auto* parentWidget = qobject_cast<QWidget*>(static_cast<QObject*>(this)->parent());
                auto response =
                    CustomMessageBox::selectable(parentWidget, QObject::tr("Managed Group Warning"),
                                                 QObject::tr("You're moving mods into managed group \"%1\".\n"
                                                             "Mods in this group can be replaced or removed on modpack update.\n\n"
                                                             "Continue?")
                                                     .arg(groupName),
                                                 QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();
                if (response != QMessageBox::Yes) {
                    return false;
                }
            }

            return m_backend->assignModsToGroup(fileKeys, targetGroupId);
        }

        if (data->hasUrls()) {
            // External files are still installed into the instance mods root. Group assignment stays virtual.
            return m_backend->dropMimeData(data, action, row, column, {});
        }

        return false;
    }

    bool isGroupIndex(const QModelIndex& index) const
    {
        auto* node = nodeForIndex(index);
        return node != nullptr && node != m_root.get() && node->kind == Node::Kind::GROUP;
    }

    QString groupIdForIndex(const QModelIndex& index) const
    {
        auto* node = nodeForIndex(index);
        if (node == nullptr || node == m_root.get()) {
            return {};
        }
        return node->groupId;
    }

    QString fileKeyForIndex(const QModelIndex& index) const
    {
        auto* node = nodeForIndex(index);
        if (node == nullptr || node->kind != Node::Kind::MOD) {
            return {};
        }
        return node->fileKey;
    }

    Resource* resourceForIndex(const QModelIndex& index) const
    {
        auto* node = nodeForIndex(index);
        if (node == nullptr || node->kind != Node::Kind::MOD) {
            return nullptr;
        }
        return node->resource;
    }

    Mod* modForIndex(const QModelIndex& index) const { return qobject_cast<Mod*>(resourceForIndex(index)); }

    QModelIndex indexForGroupId(const QString& groupId) const
    {
        auto nodeIter = m_groupNodes.constFind(groupId);
        if (nodeIter == m_groupNodes.constEnd()) {
            return {};
        }
        return indexForNode(nodeIter.value(), 0);
    }

    QModelIndex indexForFileKey(const QString& fileKey) const
    {
        auto normalizedFileKey = VirtualModGroupStore::fileKeyForFileName(fileKey);
        auto nodeIter = m_modNodesByFileKey.constFind(normalizedFileKey);
        if (nodeIter == m_modNodesByFileKey.constEnd()) {
            return {};
        }
        return indexForNode(nodeIter.value(), 0);
    }

    QList<Resource*> resourcesFromIndexes(const QModelIndexList& indexes) const
    {
        QList<Resource*> resources;
        QSet<QString> seen;

        for (auto const& index : indexes) {
            if (index.column() != 0) {
                continue;
            }
            auto* node = nodeForIndex(index);
            if (node == nullptr || node->kind != Node::Kind::MOD || node->resource == nullptr) {
                continue;
            }

            auto key = node->resource->internal_id();
            if (seen.contains(key)) {
                continue;
            }

            seen.insert(key);
            resources.append(node->resource);
        }

        return resources;
    }

    QList<Resource*> resourcesFromIndexesRecursive(const QModelIndexList& indexes) const
    {
        QList<Resource*> resources;
        QSet<QString> seen;

        for (auto const& index : indexes) {
            if (index.column() != 0) {
                continue;
            }
            collectResourcesRecursive(nodeForIndex(index), resources, seen);
        }

        return resources;
    }

    void rebuildTree()
    {
        beginResetModel();

        m_root = std::make_unique<Node>();
        m_root->kind = Node::Kind::ROOT;
        m_root->name = {};
        m_root->groupId = {};
        m_root->parent = nullptr;
        m_root->row = 0;

        m_modNodes.clear();
        m_groupNodes.clear();
        m_modNodesByFileKey.clear();

        QHash<QString, Node*> groupNodes;
        groupNodes.insert({}, m_root.get());

        auto groupOptions = m_backend->groupOptions();

        for (auto const& option : groupOptions) {
            auto groupNode = std::make_unique<Node>();
            groupNode->kind = Node::Kind::GROUP;
            groupNode->name = option.label.trimmed();
            groupNode->groupId = option.id;
            groupNode->parent = m_root.get();

            auto* groupNodePtr = groupNode.get();
            m_root->children.push_back(std::move(groupNode));
            groupNodes.insert(option.id, groupNodePtr);
            m_groupNodes.insert(option.id, groupNodePtr);
        }

        for (int row = 0; row < m_backend->rowCount(); ++row) {
            auto& resource = m_backend->at(row);
            if (resource.type() == ResourceType::FOLDER) {
                continue;
            }

            auto fileKey = VirtualModGroupStore::fileKeyForFileName(resource.getOriginalFileName());
            auto groupId = m_backend->groupForFileKey(fileKey);
            auto* parentNode = groupNodes.contains(groupId) ? groupNodes.value(groupId) : m_root.get();

            auto modNode = std::make_unique<Node>();
            modNode->kind = Node::Kind::MOD;
            modNode->name = resource.name();
            modNode->groupId = groupId;
            modNode->fileKey = fileKey;
            modNode->resource = &resource;
            modNode->backendRow = row;
            modNode->parent = parentNode;

            auto* modNodePtr = modNode.get();
            parentNode->children.push_back(std::move(modNode));
            m_modNodes.insert(resource.internal_id(), modNodePtr);
            m_modNodesByFileKey.insert(fileKey, modNodePtr);
        }

        sortChildren(m_root.get());

        endResetModel();
    }

   private:
    struct Node {
        enum class Kind : std::uint8_t {
            ROOT,
            GROUP,
            MOD,
        };

        Kind kind = Kind::ROOT;
        QString name;
        QString groupId;
        QString fileKey;
        Resource* resource = nullptr;
        int backendRow = -1;
        int row = 0;
        Node* parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;
    };

    Node* nodeForIndex(const QModelIndex& index) const
    {
        if (!index.isValid()) {
            return m_root.get();
        }
        return static_cast<Node*>(index.internalPointer());
    }

    QModelIndex indexForNode(Node* node, int column) const
    {
        if (node == nullptr || node == m_root.get()) {
            return {};
        }
        return createIndex(node->row, column, node);
    }

    static QString nodeDisplayName(const Node* node)
    {
        if (node == nullptr) {
            return {};
        }

        if (node->kind == Node::Kind::GROUP) {
            return node->name;
        }

        if (node->resource != nullptr) {
            return node->resource->name();
        }

        return node->name;
    }

    static void collectResourcesRecursive(const Node* node, QList<Resource*>& out, QSet<QString>& seen)
    {
        if (node == nullptr) {
            return;
        }

        if (node->kind == Node::Kind::MOD && node->resource != nullptr) {
            auto key = node->resource->internal_id();
            if (!seen.contains(key)) {
                seen.insert(key);
                out.append(node->resource);
            }
            return;
        }

        for (auto const& child : node->children) {
            collectResourcesRecursive(child.get(), out, seen);
        }
    }

    void sortChildren(Node* node)
    {
        if (node == nullptr) {
            return;
        }

        std::ranges::sort(node->children, [](const auto& left, const auto& right) {
            if (left->kind != right->kind) {
                return left->kind == Node::Kind::GROUP;
            }
            return QString::compare(nodeDisplayName(left.get()), nodeDisplayName(right.get()), Qt::CaseInsensitive) < 0;
        });

        for (size_t index = 0; index < node->children.size(); ++index) {
            auto* child = node->children[index].get();
            child->row = static_cast<int>(index);
            child->parent = node;
            if (child->kind == Node::Kind::GROUP) {
                sortChildren(child);
            }
        }
    }

    void onBackendDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles)
    {
        for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
            if (row < 0 || row >= m_backend->rowCount()) {
                continue;
            }

            auto& resource = m_backend->at(row);
            Node* node = nullptr;

            auto nodeIter = m_modNodes.find(resource.internal_id());
            if (nodeIter != m_modNodes.end()) {
                node = nodeIter.value();
            } else {
                for (auto iter = m_modNodes.begin(); iter != m_modNodes.end(); ++iter) {
                    if (iter.value()->resource == &resource) {
                        node = iter.value();
                        m_modNodes.erase(iter);
                        m_modNodes.insert(resource.internal_id(), node);
                        break;
                    }
                }
            }

            if (node == nullptr) {
                continue;
            }

            node->backendRow = row;
            auto leftIndex = indexForNode(node, topLeft.column());
            auto rightIndex = indexForNode(node, bottomRight.column());
            if (leftIndex.isValid() && rightIndex.isValid()) {
                emit dataChanged(leftIndex, rightIndex, roles);
            }
        }
    }

   private:
    ModFolderModel* m_backend = nullptr;
    std::unique_ptr<Node> m_root;
    QHash<QString, Node*> m_groupNodes;
    QHash<QString, Node*> m_modNodesByFileKey;
    QHash<QString, Node*> m_modNodes;
};

class VirtualModTreeProxyModel : public QSortFilterProxyModel {
   public:
    explicit VirtualModTreeProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        auto* model = dynamic_cast<VirtualModTreeModel*>(sourceModel());
        if (model == nullptr) {
            return false;
        }

        if (filterRegularExpression().pattern().isEmpty()) {
            return true;
        }

        auto nameIndex = model->index(sourceRow, ModFolderModel::NameColumn, sourceParent);
        if (!nameIndex.isValid()) {
            return false;
        }

        if (model->isGroupIndex(nameIndex)) {
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

        auto* resource = model->resourceForIndex(nameIndex);
        if (resource == nullptr) {
            return false;
        }

        return resource->applyFilter(filterRegularExpression());
    }
};

ModFolderPage::ModFolderPage(BaseInstance* inst, ModFolderModel* model, QWidget* parent)
    : ExternalResourcesPage(inst, model, parent), m_model(model)
{
    ui->actionDownloadItem->setText(tr("Download Mods"));
    ui->actionDownloadItem->setToolTip(tr("Download mods from online mod platforms"));
    ui->actionDownloadItem->setEnabled(true);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionDownloadItem);

    connect(ui->actionDownloadItem, &QAction::triggered, this, &ModFolderPage::downloadMods);

    ui->actionUpdateItem->setToolTip(tr("Try to check or update all selected mods (all mods if none are selected)"));
    connect(ui->actionUpdateItem, &QAction::triggered, this, &ModFolderPage::updateMods);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionUpdateItem);

    auto updateMenu = new QMenu(this);

    auto update = updateMenu->addAction(tr("Check for Updates"));
    connect(update, &QAction::triggered, this, &ModFolderPage::updateMods);

    updateMenu->addAction(ui->actionVerifyItemDependencies);
    connect(ui->actionVerifyItemDependencies, &QAction::triggered, this, [this] { updateMods(true); });

    auto depsDisabled = APPLICATION->settings()->getSetting("ModDependenciesDisabled");
    ui->actionVerifyItemDependencies->setVisible(!depsDisabled->get().toBool());
    connect(depsDisabled.get(), &Setting::SettingChanged, this,
            [this](const Setting&, const QVariant& value) { ui->actionVerifyItemDependencies->setVisible(!value.toBool()); });

    updateMenu->addAction(ui->actionResetItemMetadata);
    connect(ui->actionResetItemMetadata, &QAction::triggered, this, &ModFolderPage::deleteModMetadata);

    ui->actionUpdateItem->setMenu(updateMenu);

    ui->actionChangeVersion->setToolTip(tr("Change a mod's version."));
    connect(ui->actionChangeVersion, &QAction::triggered, this, &ModFolderPage::changeModVersion);
    ui->actionsToolbar->insertActionAfter(ui->actionUpdateItem, ui->actionChangeVersion);

    ui->actionViewHomepage->setToolTip(tr("View the homepages of all selected mods."));

    ui->actionExportMetadata->setToolTip(tr("Export mod's metadata to text."));
    connect(ui->actionExportMetadata, &QAction::triggered, this, &ModFolderPage::exportModMetadata);
    ui->actionsToolbar->insertActionAfter(ui->actionViewHomepage, ui->actionExportMetadata);

    ui->actionsToolbar->insertActionAfter(ui->actionViewFolder, ui->actionViewConfigs);

    if (m_model->virtualGroupsEnabled()) {
        m_actionCreateGroup = new QAction(tr("Add Group"), this);
        m_actionCreateGroup->setToolTip(tr("Create a virtual group for organizing mods."));
        connect(m_actionCreateGroup, &QAction::triggered, this, &ModFolderPage::createGroup);
        ui->actionsToolbar->insertActionAfter(ui->actionAddItem, m_actionCreateGroup);

        m_actionDeleteGroup = new QAction(tr("Delete Group"), this);
        m_actionDeleteGroup->setToolTip(tr("Delete the selected group and all mods in it."));
        connect(m_actionDeleteGroup, &QAction::triggered, this, &ModFolderPage::deleteSelectedGroup);
        ui->actionsToolbar->insertActionAfter(m_actionCreateGroup, m_actionDeleteGroup);

        m_actionMoveToGroup = new QAction(tr("Move to Group"), this);
        m_actionMoveToGroup->setToolTip(tr("Assign selected mods to a virtual group."));
        connect(m_actionMoveToGroup, &QAction::triggered, this, &ModFolderPage::moveSelectedModsToGroup);
        ui->actionsToolbar->insertActionAfter(m_actionDeleteGroup, m_actionMoveToGroup);
    }

    m_treeModel = new VirtualModTreeModel(m_model, this);
    m_treeFilterModel = new VirtualModTreeProxyModel(this);

    m_treeFilterModel->setDynamicSortFilter(true);
    m_treeFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_treeFilterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_treeFilterModel->setSourceModel(m_treeModel);
    m_treeFilterModel->setFilterKeyColumn(-1);

    m_filterModel = m_treeFilterModel;
    ui->treeView->setModel(m_filterModel);
    ui->treeView->setResizeModes(m_model->columnResizeModes());
    ui->treeView->setRootIsDecorated(true);
    ui->treeView->setItemsExpandable(true);
    ui->treeView->setExpandsOnDoubleClick(true);
    ui->treeView->setSortingEnabled(false);
    ui->treeView->setAnimated(false);
    ui->treeView->setDragDropMode(QAbstractItemView::DragDrop);

    disconnect(ui->actionRemoveItem, &QAction::triggered, this, nullptr);
    connect(ui->actionRemoveItem, &QAction::triggered, this, &ModFolderPage::removeItem);

    disconnect(ui->treeView, &ModListView::activated, this, nullptr);
    connect(ui->treeView, &ModListView::activated, this, &ModFolderPage::itemActivated);

    auto* selectionModel = ui->treeView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::currentChanged, this, &ModFolderPage::updateFrame);
    connect(selectionModel, &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex&, const QModelIndex&) { updateActions(); });

    auto updateExtra = [this]() {
        if (updateExtraInfo) {
            updateExtraInfo(id(), extraHeaderInfoString());
        }
    };
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, updateExtra);
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &ModFolderPage::updateActions);
    connect(m_treeModel, &QAbstractItemModel::modelAboutToBeReset, this, &ModFolderPage::captureTreeStateBeforeReset);
    connect(m_treeModel, &QAbstractItemModel::modelReset, this, &ModFolderPage::restoreTreeStateAfterReset);

    updateActions();
}

bool ModFolderPage::shouldDisplay() const
{
    return true;
}

QModelIndexList ModFolderPage::selectedSourceIndexes() const
{
    return m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();
}

QList<Resource*> ModFolderPage::selectedResources() const
{
    return m_treeModel->resourcesFromIndexes(selectedSourceIndexes());
}

QList<Mod*> ModFolderPage::selectedMods() const
{
    QList<Mod*> mods;
    QSet<QString> seen;
    for (auto* resource : selectedResources()) {
        auto* mod = qobject_cast<Mod*>(resource);
        if (mod == nullptr || mod->type() == ResourceType::FOLDER || mod->fileinfo().isDir()) {
            continue;
        }
        auto key = mod->internal_id();
        if (seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        mods.append(mod);
    }
    return mods;
}

QList<Resource*> ModFolderPage::resourcesForUpdateSelection(const QModelIndexList& sourceIndexes) const
{
    if (sourceIndexes.isEmpty()) {
        return m_model->allResources();
    }
    return m_treeModel->resourcesFromIndexesRecursive(sourceIndexes);
}

QModelIndexList ModFolderPage::backendIndexesForResources(const QList<Resource*>& resources) const
{
    QModelIndexList indexes;
    QSet<QString> wanted;
    for (auto* resource : resources) {
        if (resource == nullptr) {
            continue;
        }
        wanted.insert(resource->internal_id());
    }

    for (int row = 0; row < m_model->rowCount(); ++row) {
        auto backendIndex = m_model->index(row, 0);
        if (!backendIndex.isValid()) {
            continue;
        }
        auto const& resource = m_model->at(row);
        if (wanted.contains(resource.internal_id())) {
            indexes.append(backendIndex);
        }
    }

    return indexes;
}

QModelIndexList ModFolderPage::backendIndexesForGroup(const QString& groupId) const
{
    QModelIndexList indexes;
    if (groupId.isEmpty()) {
        return indexes;
    }

    for (int row = 0; row < m_model->rowCount(); ++row) {
        auto backendIndex = m_model->index(row, 0);
        if (!backendIndex.isValid()) {
            continue;
        }

        auto const& resource = m_model->at(row);
        if (resource.type() == ResourceType::FOLDER) {
            continue;
        }

        auto fileKey = VirtualModGroupStore::fileKeyForFileName(resource.getOriginalFileName());
        if (m_model->groupForFileKey(fileKey) == groupId) {
            indexes.append(backendIndex);
        }
    }

    return indexes;
}

QString ModFolderPage::currentSelectedGroupId() const
{
    return selectedGroupIdForActions();
}

QString ModFolderPage::selectedGroupIdForActions() const
{
    if (m_treeModel == nullptr || ui->treeView->selectionModel() == nullptr) {
        return {};
    }

    QSet<QString> groupIds;
    for (auto const& sourceIndex : selectedSourceIndexes()) {
        if (sourceIndex.column() != 0 || !m_treeModel->isGroupIndex(sourceIndex)) {
            continue;
        }
        auto groupId = m_treeModel->groupIdForIndex(sourceIndex);
        if (!groupId.isEmpty()) {
            groupIds.insert(groupId);
        }
    }

    if (groupIds.size() == 1) {
        return *groupIds.begin();
    }
    if (groupIds.size() > 1) {
        return {};
    }

    auto currentSource = m_filterModel->mapToSource(ui->treeView->currentIndex());
    if (!currentSource.isValid() || !m_treeModel->isGroupIndex(currentSource)) {
        return {};
    }
    return m_treeModel->groupIdForIndex(currentSource);
}

QString ModFolderPage::selectedTargetGroupId() const
{
    if (m_treeModel == nullptr || ui->treeView->selectionModel() == nullptr) {
        return {};
    }

    QSet<QString> groupIds;
    for (auto const& sourceIndex : selectedSourceIndexes()) {
        if (sourceIndex.column() != 0 || !m_treeModel->isGroupIndex(sourceIndex)) {
            continue;
        }
        auto groupId = m_treeModel->groupIdForIndex(sourceIndex);
        if (!groupId.isEmpty()) {
            groupIds.insert(groupId);
        }
    }

    if (groupIds.size() != 1) {
        return {};
    }

    return *groupIds.begin();
}

QString ModFolderPage::groupDisplayName(const QString& groupId) const
{
    if (groupId.isEmpty()) {
        return tr("Ungrouped mods");
    }

    for (auto const& option : m_model->groupOptions()) {
        if (option.id == groupId) {
            auto label = option.label.trimmed();
            if (!label.isEmpty()) {
                return label;
            }
            break;
        }
    }

    return tr("selected group");
}

bool ModFolderPage::confirmManagedGroupModification(const QString& groupId, const QString& actionDescription)
{
    if (groupId.isEmpty() || !m_model->isManagedGroup(groupId)) {
        return true;
    }

    auto response = CustomMessageBox::selectable(this, tr("Managed Group Warning"),
                                                 tr("You're about to %1 in managed group \"%2\".\n"
                                                    "Mods in this group can be replaced or removed on modpack update.\n\n"
                                                    "Continue?")
                                                     .arg(actionDescription, groupDisplayName(groupId)),
                                                 QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();
    return response == QMessageBox::Yes;
}

void ModFolderPage::updateActions()
{
    auto resources = selectedResources();
    bool hasModSelection = !resources.isEmpty();
    auto selectedGroupId = currentSelectedGroupId();
    bool hasGroupSelection = !selectedGroupId.isEmpty();

    ui->actionUpdateItem->setEnabled(!m_model->empty());
    ui->actionResetItemMetadata->setEnabled(hasModSelection);

    ui->actionChangeVersion->setEnabled(resources.size() == 1 && resources[0]->metadata() != nullptr);

    ui->actionRemoveItem->setEnabled(hasModSelection);
    ui->actionEnableItem->setEnabled(hasModSelection);
    ui->actionDisableItem->setEnabled(hasModSelection);

    ui->actionViewHomepage->setEnabled(hasModSelection && std::ranges::any_of(resources, [](Resource* resource) {
                                           return resource != nullptr && !resource->homepage().isEmpty();
                                       }));
    ui->actionExportMetadata->setEnabled(!m_model->empty());

    if (m_actionCreateGroup != nullptr) {
        m_actionCreateGroup->setEnabled(m_model->virtualGroupsEnabled() && !hasGroupSelection);
    }

    if (m_actionDeleteGroup != nullptr) {
        m_actionDeleteGroup->setEnabled(m_model->virtualGroupsEnabled() && hasGroupSelection);
    }

    if (m_actionMoveToGroup != nullptr) {
        m_actionMoveToGroup->setEnabled(m_model->virtualGroupsEnabled() && hasModSelection);
    }
}

void ModFolderPage::updateFrame(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    auto sourceCurrent = m_filterModel->mapToSource(current);
    if (!sourceCurrent.isValid()) {
        ui->frame->clear();
        return;
    }

    auto* mod = m_treeModel->modForIndex(sourceCurrent);
    if (mod == nullptr) {
        ui->frame->clear();
        return;
    }

    ui->frame->updateWithMod(*mod);
}

void ModFolderPage::captureTreeStateBeforeReset()
{
    m_pendingSelectedGroupIds.clear();
    m_pendingSelectedFileKeys.clear();
    m_pendingExpandedGroupIds.clear();
    m_pendingScrollValid = false;
    m_pendingVerticalScrollValue = 0;
    m_pendingHorizontalScrollValue = 0;

    if (m_treeModel == nullptr || m_filterModel == nullptr || ui->treeView->selectionModel() == nullptr) {
        return;
    }

    QSet<QString> selectedGroupIds;
    QSet<QString> selectedFileKeys;
    auto sourceSelection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();
    for (auto const& sourceIndex : sourceSelection) {
        if (sourceIndex.column() != 0) {
            continue;
        }

        if (m_treeModel->isGroupIndex(sourceIndex)) {
            auto groupId = m_treeModel->groupIdForIndex(sourceIndex);
            if (!groupId.isEmpty()) {
                selectedGroupIds.insert(groupId);
            }
            continue;
        }

        auto fileKey = m_treeModel->fileKeyForIndex(sourceIndex);
        if (!fileKey.isEmpty()) {
            selectedFileKeys.insert(fileKey);
        }
    }

    QSet<QString> expandedGroupIds;
    for (auto const& option : m_model->groupOptions()) {
        auto sourceIndex = m_treeModel->indexForGroupId(option.id);
        if (!sourceIndex.isValid()) {
            continue;
        }

        auto proxyIndex = m_filterModel->mapFromSource(sourceIndex);
        if (proxyIndex.isValid() && ui->treeView->isExpanded(proxyIndex)) {
            expandedGroupIds.insert(option.id);
        }
    }

    m_pendingSelectedGroupIds = selectedGroupIds.values();
    m_pendingSelectedFileKeys = selectedFileKeys.values();
    m_pendingExpandedGroupIds = expandedGroupIds.values();

    if (ui->treeView->verticalScrollBar() != nullptr && ui->treeView->horizontalScrollBar() != nullptr) {
        m_pendingScrollValid = true;
        m_pendingVerticalScrollValue = ui->treeView->verticalScrollBar()->value();
        m_pendingHorizontalScrollValue = ui->treeView->horizontalScrollBar()->value();
    }
}

void ModFolderPage::restoreTreeStateAfterReset()
{
    if (m_treeModel == nullptr || m_filterModel == nullptr || ui->treeView->selectionModel() == nullptr) {
        m_pendingSelectedGroupIds.clear();
        m_pendingSelectedFileKeys.clear();
        m_pendingExpandedGroupIds.clear();
        return;
    }

    for (auto const& groupId : m_pendingExpandedGroupIds) {
        auto sourceIndex = m_treeModel->indexForGroupId(groupId);
        if (!sourceIndex.isValid()) {
            continue;
        }

        auto proxyIndex = m_filterModel->mapFromSource(sourceIndex);
        if (proxyIndex.isValid()) {
            ui->treeView->setExpanded(proxyIndex, true);
        }
    }

    QItemSelection selection;
    QModelIndex firstProxyIndex;
    auto selectIndex = [this, &selection, &firstProxyIndex](const QModelIndex& sourceIndex) {
        if (!sourceIndex.isValid()) {
            return;
        }

        auto proxyIndex = m_filterModel->mapFromSource(sourceIndex);
        if (!proxyIndex.isValid()) {
            return;
        }

        auto firstColumn = proxyIndex.sibling(proxyIndex.row(), 0);
        auto lastColumn = proxyIndex.sibling(proxyIndex.row(), m_filterModel->columnCount() - 1);
        selection.select(firstColumn, lastColumn);
        if (!firstProxyIndex.isValid()) {
            firstProxyIndex = firstColumn;
        }
    };

    for (auto const& groupId : m_pendingSelectedGroupIds) {
        selectIndex(m_treeModel->indexForGroupId(groupId));
    }
    for (auto const& fileKey : m_pendingSelectedFileKeys) {
        selectIndex(m_treeModel->indexForFileKey(fileKey));
    }

    auto* selectionModel = ui->treeView->selectionModel();
    if (!selection.indexes().isEmpty()) {
        selectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        if (firstProxyIndex.isValid()) {
            selectionModel->setCurrentIndex(firstProxyIndex, QItemSelectionModel::NoUpdate);
        }
    }

    m_pendingSelectedGroupIds.clear();
    m_pendingSelectedFileKeys.clear();
    m_pendingExpandedGroupIds.clear();
    if (m_pendingScrollValid) {
        auto verticalValue = m_pendingVerticalScrollValue;
        auto horizontalValue = m_pendingHorizontalScrollValue;
        QTimer::singleShot(0, this, [this, verticalValue, horizontalValue]() {
            if (ui == nullptr || ui->treeView == nullptr || ui->treeView->verticalScrollBar() == nullptr ||
                ui->treeView->horizontalScrollBar() == nullptr) {
                return;
            }

            ui->treeView->verticalScrollBar()->setValue(verticalValue);
            ui->treeView->horizontalScrollBar()->setValue(horizontalValue);
        });
    }
    m_pendingScrollValid = false;
    m_pendingVerticalScrollValue = 0;
    m_pendingHorizontalScrollValue = 0;
    updateActions();
}

void ModFolderPage::removeItem()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    auto resources = m_treeModel->resourcesFromIndexes(selection.indexes());

    if (resources.isEmpty()) {
        return;
    }

    QString text;
    bool multiple = resources.size() > 1;

    if (multiple) {
        text = tr("You are about to remove %1 items.\n"
                  "This may be permanent and they will be gone from the folder.\n\n"
                  "Are you sure?")
                   .arg(resources.size());
    }

    if (!text.isEmpty()) {
        auto response = CustomMessageBox::selectable(this, tr("Confirm Removal"), text, QMessageBox::Warning,
                                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                            ->exec();

        if (response != QMessageBox::Yes) {
            return;
        }
    }

    removeItems(selection);
}

void ModFolderPage::removeItems(const QItemSelection& selection)
{
    if (m_instance != nullptr && m_instance->isRunning()) {
        auto response = CustomMessageBox::selectable(this, tr("Confirm Delete"),
                                                     tr("If you remove mods while the game is running it may crash your game.\n"
                                                        "Are you sure you want to do this?"),
                                                     QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                            ->exec();

        if (response != QMessageBox::Yes) {
            return;
        }
    }

    auto resources = m_treeModel->resourcesFromIndexes(selection.indexes());
    auto indexes = backendIndexesForResources(resources);
    if (indexes.isEmpty()) {
        return;
    }

    auto affected = m_model->getAffectedMods(indexes, EnableAction::DISABLE);
    if (!affected.isEmpty()) {
        auto response = CustomMessageBox::selectable(this, tr("Confirm Disable"),
                                                     tr("The mods you are trying to delete are required by %1 mods.\n"
                                                        "Do you want to disable them?")
                                                         .arg(affected.length()),
                                                     QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                                     QMessageBox::Cancel)
                            ->exec();

        if (response != QMessageBox::Yes) {
            m_model->setResourceEnabled(affected, EnableAction::DISABLE);
        } else if (response != QMessageBox::Cancel) {
            return;
        }
    }

    m_model->deleteResources(indexes);
}

void ModFolderPage::enableItem()
{
    auto indexes = backendIndexesForResources(selectedResources());
    m_model->setResourceEnabled(indexes, EnableAction::ENABLE);
}

void ModFolderPage::disableItem()
{
    auto indexes = backendIndexesForResources(selectedResources());
    m_model->setResourceEnabled(indexes, EnableAction::DISABLE);
}

void ModFolderPage::viewHomepage()
{
    for (auto* resource : selectedResources()) {
        if (resource == nullptr) {
            continue;
        }
        auto url = resource->homepage();
        if (!url.isEmpty()) {
            DesktopServices::openUrl(url);
        }
    }
}

void ModFolderPage::itemActivated(const QModelIndex& index)
{
    auto sourceIndex = m_filterModel->mapToSource(index);
    if (!sourceIndex.isValid() || m_treeModel->isGroupIndex(sourceIndex)) {
        return;
    }

    auto indexes = backendIndexesForResources(selectedResources());
    if (indexes.isEmpty()) {
        return;
    }

    m_model->setResourceEnabled(indexes, EnableAction::TOGGLE);
}

bool ModFolderPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress && obj == ui->treeView) {
        auto* keyEvent = static_cast<QKeyEvent*>(ev);
        switch (keyEvent->key()) {
            case Qt::Key_Delete:
                removeItem();
                return true;
            case Qt::Key_Plus:
                addItem();
                return true;
            default:
                break;
        }
    }

    return ExternalResourcesPage::eventFilter(obj, ev);
}

void ModFolderPage::downloadMods()
{
    if (m_instance->typeName() != "Minecraft")
        return;  // this is a null instance or a legacy instance

    auto profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
    if (!profile->getModLoaders().has_value()) {
        if (handleNoModLoader()) {
            return;
        }
    }

    if (m_downloadUpdateConnection) {
        disconnect(m_downloadUpdateConnection);
        m_downloadUpdateConnection = {};
    }
    m_preDownloadFileKeys.clear();
    m_pendingDownloadGroupId.clear();
    if (m_model->virtualGroupsEnabled()) {
        auto selectedGroupId = selectedTargetGroupId();
        if (!confirmManagedGroupModification(selectedGroupId, tr("download mods"))) {
            return;
        }
        m_pendingDownloadGroupId = selectedGroupId;
        m_preDownloadFileKeys = collectCurrentModFileKeys(m_model);
    }

    m_downloadDialog = new ResourceDownload::ModDownloadDialog(this, m_model, m_instance);
    connect(this, &QObject::destroyed, m_downloadDialog, &QDialog::close);
    connect(m_downloadDialog, &QDialog::finished, this, &ModFolderPage::downloadDialogFinished);

    m_downloadDialog->open();
}

void ModFolderPage::downloadDialogFinished(int result)
{
    if (result) {
        auto tasks = new ConcurrentTask(tr("Download Mods"), APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        connect(tasks, &Task::failed, [this, tasks](QString reason) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::aborted, [this, tasks]() {
            CustomMessageBox::selectable(this, tr("Aborted"), tr("Download stopped by user."), QMessageBox::Information)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::succeeded, [this, tasks]() {
            QStringList warnings = tasks->warnings();
            if (warnings.count()) {
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }

            tasks->deleteLater();
        });

        if (m_downloadDialog) {
            for (auto& task : m_downloadDialog->getTasks()) {
                tasks->addTask(task);
            }
        } else {
            qWarning() << "ResourceDownloadDialog vanished before we could collect tasks!";
        }

        ProgressDialog loadDialog(this);
        loadDialog.setSkipButton(true, tr("Abort"));
        loadDialog.execWithTask(tasks);

        if (m_downloadUpdateConnection) {
            disconnect(m_downloadUpdateConnection);
            m_downloadUpdateConnection = {};
        }

        if (m_model->virtualGroupsEnabled() && !m_pendingDownloadGroupId.isEmpty()) {
            auto existingFileKeys = m_preDownloadFileKeys;
            auto targetGroupId = m_pendingDownloadGroupId;
            m_downloadUpdateConnection =
                connect(m_model, &ResourceFolderModel::updateFinished, this, [this, existingFileKeys, targetGroupId]() {
                    if (m_downloadUpdateConnection) {
                        disconnect(m_downloadUpdateConnection);
                        m_downloadUpdateConnection = {};
                    }

                    QStringList newFileKeys;
                    auto currentFileKeys = collectCurrentModFileKeys(m_model);
                    for (auto const& fileKey : currentFileKeys) {
                        if (!existingFileKeys.contains(fileKey)) {
                            newFileKeys.push_back(fileKey);
                        }
                    }

                    if (!newFileKeys.isEmpty() && !m_model->assignModsToGroup(newFileKeys, targetGroupId)) {
                        qWarning() << "Could not assign downloaded mods to group:" << targetGroupId;
                    }

                    m_preDownloadFileKeys.clear();
                    m_pendingDownloadGroupId.clear();
                });
        } else {
            m_preDownloadFileKeys.clear();
            m_pendingDownloadGroupId.clear();
        }

        m_model->update();
    } else {
        if (m_downloadUpdateConnection) {
            disconnect(m_downloadUpdateConnection);
            m_downloadUpdateConnection = {};
        }
        m_preDownloadFileKeys.clear();
        m_pendingDownloadGroupId.clear();
    }
    if (m_downloadDialog) {
        m_downloadDialog->deleteLater();
    }
}

void ModFolderPage::updateMods(bool includeDeps)
{
    if (m_instance->typeName() != "Minecraft")
        return;  // this is a null instance or a legacy instance

    auto profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
    if (!profile->getModLoaders().has_value()) {
        if (handleNoModLoader()) {
            return;
        }
    }
    if (APPLICATION->settings()->get("ModMetadataDisabled").toBool()) {
        QMessageBox::critical(this, tr("Error"), tr("Mod updates are unavailable when metadata is disabled!"));
        return;
    }
    if (m_instance != nullptr && m_instance->isRunning()) {
        auto response =
            CustomMessageBox::selectable(this, tr("Confirm Update"),
                                         tr("Updating mods while the game is running may cause mod duplication and game crashes.\n"
                                            "The old files may not be deleted as they are in use.\n"
                                            "Are you sure you want to do this?"),
                                         QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                ->exec();

        if (response != QMessageBox::Yes)
            return;
    }

    auto selection = selectedSourceIndexes();
    bool useAll = selection.isEmpty();
    auto modsList = resourcesForUpdateSelection(selection);

    if (modsList.empty()) {
        CustomMessageBox::selectable(this, tr("Update checker"), tr("No mods are available for updates."))->exec();
        return;
    }

    ResourceUpdateDialog updateDialog(this, m_instance, m_model, modsList, includeDeps, profile->getModLoadersList(), useAll);
    updateDialog.checkCandidates();

    if (updateDialog.aborted()) {
        CustomMessageBox::selectable(this, tr("Aborted"), tr("The mod updater was aborted!"), QMessageBox::Warning)->show();
        return;
    }
    if (updateDialog.noUpdates()) {
        QString message{ tr("'%1' is up-to-date! :)").arg(modsList.front()->name()) };
        if (modsList.size() > 1) {
            if (useAll) {
                message = tr("All mods are up-to-date! :)");
            } else {
                message = tr("All selected mods are up-to-date! :)");
            }
        }
        CustomMessageBox::selectable(this, tr("Update checker"), message)->exec();
        return;
    }

    if (updateDialog.exec()) {
        auto tasks = new ConcurrentTask("Download Mods", APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        connect(tasks, &Task::failed, [this, tasks](QString reason) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::aborted, [this, tasks]() {
            CustomMessageBox::selectable(this, tr("Aborted"), tr("Download stopped by user."), QMessageBox::Information)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::succeeded, [this, tasks]() {
            QStringList warnings = tasks->warnings();
            if (warnings.count()) {
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }
            tasks->deleteLater();
        });

        for (auto task : updateDialog.getTasks()) {
            tasks->addTask(task);
        }

        ProgressDialog loadDialog(this);
        loadDialog.setSkipButton(true, tr("Abort"));
        loadDialog.execWithTask(tasks);

        m_model->update();
    }
}

void ModFolderPage::deleteModMetadata()
{
    auto selectionCount = selectedMods().length();
    if (selectionCount == 0)
        return;
    if (selectionCount > 1) {
        auto response = CustomMessageBox::selectable(this, tr("Confirm Removal"),
                                                     tr("You are about to remove the metadata for %1 mods.\n"
                                                        "Are you sure?")
                                                         .arg(selectionCount),
                                                     QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                            ->exec();

        if (response != QMessageBox::Yes)
            return;
    }

    m_model->deleteMetadata(backendIndexesForResources(selectedResources()));
}

void ModFolderPage::changeModVersion()
{
    if (m_instance->typeName() != "Minecraft")
        return;  // this is a null instance or a legacy instance

    auto profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
    if (!profile->getModLoaders().has_value()) {
        if (handleNoModLoader()) {
            return;
        }
    }
    if (APPLICATION->settings()->get("ModMetadataDisabled").toBool()) {
        QMessageBox::critical(this, tr("Error"), tr("Mod updates are unavailable when metadata is disabled!"));
        return;
    }

    auto modsList = selectedMods();
    if (modsList.length() != 1 || modsList[0]->metadata() == nullptr)
        return;

    if (m_downloadUpdateConnection) {
        disconnect(m_downloadUpdateConnection);
        m_downloadUpdateConnection = {};
    }
    m_preDownloadFileKeys.clear();
    m_pendingDownloadGroupId.clear();
    if (m_model->virtualGroupsEnabled()) {
        auto selectedGroupId = selectedTargetGroupId();
        if (!confirmManagedGroupModification(selectedGroupId, tr("download mods"))) {
            return;
        }
        m_pendingDownloadGroupId = selectedGroupId;
        m_preDownloadFileKeys = collectCurrentModFileKeys(m_model);
    }

    m_downloadDialog = new ResourceDownload::ModDownloadDialog(this, m_model, m_instance);
    connect(this, &QObject::destroyed, m_downloadDialog, &QDialog::close);
    connect(m_downloadDialog, &QDialog::finished, this, &ModFolderPage::downloadDialogFinished);

    m_downloadDialog->setResourceMetadata((*modsList.begin())->metadata());
    m_downloadDialog->open();
}

void ModFolderPage::exportModMetadata()
{
    auto selectedMods = this->selectedMods();
    if (selectedMods.length() == 0)
        selectedMods = m_model->allMods();

    std::sort(selectedMods.begin(), selectedMods.end(), [](const Mod* a, const Mod* b) { return a->name() < b->name(); });
    ExportToModListDialog dlg(m_instance->name(), selectedMods, this);
    dlg.exec();
}

void ModFolderPage::createGroup()
{
    if (!m_model->virtualGroupsEnabled()) {
        return;
    }

    bool accepted = false;
    auto groupName = QInputDialog::getText(this, tr("Create Group"), tr("Group name:"), QLineEdit::Normal, QString(), &accepted).trimmed();
    if (!accepted || groupName.isEmpty()) {
        return;
    }

    if (!m_model->createGroup(groupName, {})) {
        CustomMessageBox::selectable(this, tr("Error"), tr("Could not create group."), QMessageBox::Critical)->show();
    }
}

void ModFolderPage::deleteSelectedGroup()
{
    if (!m_model->virtualGroupsEnabled()) {
        return;
    }

    auto groupId = currentSelectedGroupId();
    if (groupId.isEmpty()) {
        return;
    }
    if (!confirmManagedGroupModification(groupId, tr("delete mods"))) {
        return;
    }

    auto groupModIndexes = backendIndexesForGroup(groupId);
    auto currentIndex = ui->treeView->currentIndex();
    auto groupName = currentIndex.sibling(currentIndex.row(), ModFolderModel::NameColumn).data().toString().trimmed();
    if (groupName.isEmpty()) {
        groupName = tr("selected group");
    }

    auto response = CustomMessageBox::selectable(this, tr("Delete Group"),
                                                 tr("Delete group \"%1\" and permanently delete all %2 mod(s) in this group?")
                                                     .arg(groupName, QString::number(groupModIndexes.size())),
                                                 QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();
    if (response != QMessageBox::Yes) {
        return;
    }

    if (!groupModIndexes.isEmpty() && !m_model->deleteResources(groupModIndexes)) {
        CustomMessageBox::selectable(this, tr("Error"), tr("Could not delete mods in the selected group."), QMessageBox::Critical)->show();
        return;
    }

    if (!m_model->deleteGroup(groupId)) {
        CustomMessageBox::selectable(this, tr("Error"), tr("Could not delete group."), QMessageBox::Critical)->show();
    }
}

void ModFolderPage::moveSelectedModsToGroup()
{
    if (!m_model->virtualGroupsEnabled()) {
        return;
    }

    QStringList fileKeys;
    for (auto* resource : selectedResources()) {
        if (resource == nullptr || resource->type() == ResourceType::FOLDER) {
            continue;
        }
        auto fileKey = VirtualModGroupStore::fileKeyForFileName(resource->getOriginalFileName());
        if (!fileKey.isEmpty()) {
            fileKeys.append(fileKey);
        }
    }
    fileKeys.removeDuplicates();

    if (fileKeys.isEmpty()) {
        return;
    }

    QStringList labels;
    labels.append(tr("(Ungrouped)"));
    auto groupOptions = m_model->groupOptions();
    for (auto const& option : groupOptions) {
        labels.append(option.label);
    }

    bool accepted = false;
    auto selectedLabel = QInputDialog::getItem(this, tr("Assign Group"), tr("Move selected mods to group:"), labels, 0, false, &accepted);
    if (!accepted) {
        return;
    }

    QString targetGroupId;
    if (selectedLabel != tr("(Ungrouped)")) {
        for (auto const& option : groupOptions) {
            if (option.label == selectedLabel) {
                targetGroupId = option.id;
                break;
            }
        }
    }

    if (!confirmManagedGroupModification(targetGroupId, tr("move mods"))) {
        return;
    }

    if (!m_model->assignModsToGroup(fileKeys, targetGroupId)) {
        CustomMessageBox::selectable(this, tr("Error"), tr("Could not move selected mods to the selected group."), QMessageBox::Critical)
            ->show();
    }
}

CoreModFolderPage::CoreModFolderPage(BaseInstance* inst, ModFolderModel* mods, QWidget* parent) : ModFolderPage(inst, mods, parent)
{
    auto mcInst = dynamic_cast<MinecraftInstance*>(m_instance);
    if (mcInst) {
        auto version = mcInst->getPackProfile();
        if (version && version->getComponent("net.minecraftforge") && version->getComponent("net.minecraft")) {
            auto minecraftCmp = version->getComponent("net.minecraft");
            if (!minecraftCmp->m_loaded) {
                version->reload(Net::Mode::Offline);
                auto update = version->getCurrentTask();
                if (update) {
                    connect(update.get(), &Task::finished, this, [this] {
                        if (m_container) {
                            m_container->refreshContainer();
                        }
                    });
                    if (!update->isRunning()) {
                        update->start();
                    }
                }
            }
        }
    }
}

bool CoreModFolderPage::shouldDisplay() const
{
    if (ModFolderPage::shouldDisplay()) {
        auto inst = dynamic_cast<MinecraftInstance*>(m_instance);
        if (!inst)
            return true;

        auto version = inst->getPackProfile();
        if (!version || !version->getComponent("net.minecraftforge") || !version->getComponent("net.minecraft"))
            return false;
        auto minecraftCmp = version->getComponent("net.minecraft");
        return minecraftCmp->m_loaded && minecraftCmp->getReleaseDateTime() < g_VersionFilterData.legacyCutoffDate;
    }
    return false;
}

NilModFolderPage::NilModFolderPage(BaseInstance* inst, ModFolderModel* mods, QWidget* parent) : ModFolderPage(inst, mods, parent) {}

bool NilModFolderPage::shouldDisplay() const
{
    return m_model->dir().exists();
}

// Helper function so this doesn't need to be duplicated 3 times
inline bool ModFolderPage::handleNoModLoader()
{
    int resp =
        QMessageBox::question(this, this->tr("Missing Mod Loader"),
                              this->tr("You need to install a compatible mod loader before installing mods. Would you like to do so?"),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    switch (resp) {
        case QMessageBox::Yes: {
            // Should be safe
            auto profile = static_cast<MinecraftInstance*>(this->m_instance)->getPackProfile();
            InstallLoaderDialog dialog(profile, QString(), this);
            bool ret = dialog.exec();
            this->m_container->refreshContainer();

            // returning negation of dialog.exec which'll be true if the install loader dialog got canceled/closed
            // and false if the user went through and installed a loader
            return !ret;
        }
        case QMessageBox::No: {
            // Nothing happens the dialog is already closing
            // returning true so the caller doesn't go and continue with opening it's dialog without a mod loader
            return true;
        }
        default: {
            // Unreachable
            // returning true as a safety measure
            return true;
        }
    }
}
