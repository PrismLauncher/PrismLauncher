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
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QString>
#include <QStyle>
#include <QThreadPool>
#include <QUrl>
#include <QUuid>
#include <algorithm>

#include "minecraft/Component.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/Resource.h"
#include "minecraft/mod/ResourceFolderModel.h"
#include "minecraft/mod/VirtualModGroupStore.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "modplatform/ModIndex.h"
#include "ui/dialogs/CustomMessageBox.h"

namespace {

[[nodiscard]] QString normalizedPath(const QString& path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

[[nodiscard]] bool isMainModsDirectory(const QDir& candidateDir, BaseInstance* instance)
{
    auto* mcInstance = dynamic_cast<MinecraftInstance*>(instance);
    if (mcInstance == nullptr) {
        return false;
    }

    return normalizedPath(candidateDir.absolutePath()) == normalizedPath(mcInstance->modsRoot());
}

[[nodiscard]] QSet<QString> parseModrinthManagedFileKeys(const QString& manifestPath)
{
    QSet<QString> fileKeys;

    QFile manifestFile(manifestPath);
    if (!manifestFile.exists() || !manifestFile.open(QIODevice::ReadOnly)) {
        return fileKeys;
    }

    QJsonParseError parseError{};
    auto document = QJsonDocument::fromJson(manifestFile.readAll(), &parseError);
    manifestFile.close();
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return fileKeys;
    }

    auto files = document.object().value("files");
    if (!files.isArray()) {
        return fileKeys;
    }

    for (auto const& fileEntry : files.toArray()) {
        if (!fileEntry.isObject()) {
            continue;
        }

        auto path = fileEntry.toObject().value("path").toString();
        if (!path.startsWith("mods/")) {
            continue;
        }

        auto fileName = QFileInfo(FS::RemoveInvalidPathChars(path)).fileName();
        fileKeys.insert(VirtualModGroupStore::fileKeyForFileName(fileName));
    }

    return fileKeys;
}

[[nodiscard]] QSet<QString> parseFlameManagedProjectIds(const QString& manifestPath)
{
    QSet<QString> projectIds;

    QFile manifestFile(manifestPath);
    if (!manifestFile.exists() || !manifestFile.open(QIODevice::ReadOnly)) {
        return projectIds;
    }

    QJsonParseError parseError{};
    auto document = QJsonDocument::fromJson(manifestFile.readAll(), &parseError);
    manifestFile.close();
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return projectIds;
    }

    auto files = document.object().value("files");
    if (!files.isArray()) {
        return projectIds;
    }

    for (auto const& fileEntry : files.toArray()) {
        if (!fileEntry.isObject()) {
            continue;
        }
        auto object = fileEntry.toObject();
        auto projectId = object.value("projectID").toVariant().toString();
        if (!projectId.isEmpty()) {
            projectIds.insert(projectId);
        }
    }

    return projectIds;
}

[[nodiscard]] bool areSameVirtualEntries(const VirtualModGroupStore::Entry& left, const VirtualModGroupStore::Entry& right)
{
    return left.fileKey == right.fileKey && left.fileName == right.fileName && left.groupId == right.groupId &&
           left.sourceType == right.sourceType && left.linkedMetadataSlug == right.linkedMetadataSlug && left.provider == right.provider &&
           left.projectId == right.projectId;
}

[[nodiscard]] QHash<QString, VirtualModGroupStore::Entry> virtualEntryMapByKey(const QList<VirtualModGroupStore::Entry>& entries)
{
    QHash<QString, VirtualModGroupStore::Entry> map;
    map.reserve(entries.size());
    for (auto const& entry : entries) {
        map.insert(entry.fileKey, entry);
    }
    return map;
}

[[nodiscard]] bool areSameVirtualEntryMaps(const QHash<QString, VirtualModGroupStore::Entry>& left,
                                           const QHash<QString, VirtualModGroupStore::Entry>& right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (auto it = left.constBegin(); it != left.constEnd(); ++it) {
        auto rightIt = right.constFind(it.key());
        if (rightIt == right.constEnd() || !areSameVirtualEntries(it.value(), rightIt.value())) {
            return false;
        }
    }

    return true;
}

struct GroupCarryHint {
    QString groupId;
    VirtualModGroupStore::SourceType sourceType = VirtualModGroupStore::SourceType::LOCAL_NO_SOURCE;
};

[[nodiscard]] QString groupCarryProjectIdentity(const VirtualModGroupStore::Entry& entry)
{
    auto provider = entry.provider.trimmed().toLower();
    auto projectId = entry.projectId.trimmed();
    if (provider.isEmpty() || projectId.isEmpty()) {
        return {};
    }
    return provider + "|" + projectId;
}

[[nodiscard]] QString groupCarrySlugIdentity(const VirtualModGroupStore::Entry& entry)
{
    auto provider = entry.provider.trimmed().toLower();
    auto slug = entry.linkedMetadataSlug.trimmed().toLower();
    if (provider.isEmpty() || slug.isEmpty()) {
        return {};
    }
    return provider + "|" + slug;
}

void recordGroupCarryHint(const VirtualModGroupStore::Entry& entry,
                          const QString& identity,
                          QHash<QString, GroupCarryHint>& hints,
                          QSet<QString>& ambiguousIdentities)
{
    if (identity.isEmpty() || entry.groupId.isEmpty() || ambiguousIdentities.contains(identity)) {
        return;
    }

    auto hintIter = hints.find(identity);
    if (hintIter == hints.end()) {
        hints.insert(identity, { entry.groupId, entry.sourceType });
        return;
    }

    if (hintIter->groupId != entry.groupId) {
        hints.remove(identity);
        ambiguousIdentities.insert(identity);
        return;
    }

    if (hintIter->sourceType != VirtualModGroupStore::SourceType::MANAGED_PACK &&
        entry.sourceType == VirtualModGroupStore::SourceType::MANAGED_PACK) {
        hintIter->sourceType = VirtualModGroupStore::SourceType::MANAGED_PACK;
    }
}

void buildGroupCarryHints(const QHash<QString, VirtualModGroupStore::Entry>& beforeEntries,
                          QHash<QString, GroupCarryHint>& projectHints,
                          QHash<QString, GroupCarryHint>& slugHints)
{
    QSet<QString> ambiguousProjectIdentities;
    QSet<QString> ambiguousSlugIdentities;

    for (auto const& entry : beforeEntries) {
        if (entry.groupId.isEmpty()) {
            continue;
        }

        recordGroupCarryHint(entry, groupCarryProjectIdentity(entry), projectHints, ambiguousProjectIdentities);
        recordGroupCarryHint(entry, groupCarrySlugIdentity(entry), slugHints, ambiguousSlugIdentities);
    }
}

}  // namespace

ModFolderModel::ModFolderModel(const QDir& dir, BaseInstance* instance, bool is_indexed, bool create_dir, QObject* parent)
    : ResourceFolderModel(QDir(dir), instance, is_indexed, create_dir, parent)
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

    m_virtualGroupsEnabled = isMainModsDirectory(m_dir, instance);

    if (m_virtualGroupsEnabled) {
        m_groupStore = std::make_unique<VirtualModGroupStore>(m_dir, indexDir());
        initializeVirtualGroups();
    }

    connect(this, &ModFolderModel::parseFinished, this, &ModFolderModel::onParseFinished);
}

ModFolderModel::~ModFolderModel() = default;

QVariant ModFolderModel::data(const QModelIndex& index, int role) const
{
    if (!validateIndex(index))
        return {};

    int row = index.row();
    int column = index.column();

    switch (role) {
        case Qt::DisplayRole:
            switch (column) {
                case NameColumn:
                    return m_resources[row]->name();
                case VersionColumn: {
                    switch (at(row).type()) {
                        case ResourceType::FOLDER:
                            return tr("Folder");
                        case ResourceType::SINGLEFILE:
                            return tr("File");
                        default:
                            break;
                    }
                    return at(row).version();
                }
                case DateColumn:
                    return at(row).dateTimeChanged();
                case ProviderColumn: {
                    return at(row).provider();
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
                case SizeColumn: {
                    return at(row).sizeStr();
                }
                case RequiredByColumn: {
                    return at(row).requiredByCount();
                }
                case RequiresColumn: {
                    return at(row).requiresCount();
                }
                default:
                    return QVariant();
            }

        case Qt::ToolTipRole:
            if (column == NameColumn) {
                if (at(row).isSymLinkUnder(instDirPath())) {
                    return m_resources[row]->internal_id() +
                           tr("\nWarning: This resource is symbolically linked from elsewhere. Editing it will also change the original."
                              "\nCanonical Path: %1")
                               .arg(at(row).fileinfo().canonicalFilePath());
                }
                if (at(row).isMoreThanOneHardLink()) {
                    return m_resources[row]->internal_id() +
                           tr("\nWarning: This resource is hard linked elsewhere. Editing it will also change the original.");
                }
            }
            return m_resources[row]->internal_id();
        case Qt::DecorationRole: {
            if (column == NameColumn && (at(row).isSymLinkUnder(instDirPath()) || at(row).isMoreThanOneHardLink()))
                return QIcon::fromTheme("status-yellow");
            if (column == ImageColumn) {
                return at(row).icon({ 32, 32 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding);
            }
            return {};
        }
        case Qt::SizeHintRole:
            if (column == ImageColumn) {
                return QSize(32, 32);
            }
            return {};
        case Qt::CheckStateRole:
            if (column == ActiveColumn)
                return at(row).enabled() ? Qt::Checked : Qt::Unchecked;
            return QVariant();
        default:
            return QVariant();
    }
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
    return parent.isValid() ? 0 : NUM_COLUMNS;
}

QSortFilterProxyModel* ModFolderModel::createFilterProxyModel(QObject* parent)
{
    return new ProxyModel(parent);
}

Task* ModFolderModel::createParseTask(Resource& resource)
{
    return new LocalModParseTask(m_next_resolution_ticket, resource.type(), resource.fileinfo());
}

bool ModFolderModel::isValid()
{
    return m_dir.exists() && m_dir.isReadable();
}

void ModFolderModel::onUpdateSucceeded()
{
    ResourceFolderModel::onUpdateSucceeded();

    if (m_virtualGroupsEnabled && m_groupStore != nullptr) {
        bool reloadedGroupState = false;
        if (m_groupStore->exists()) {
            if (!m_groupStore->load()) {
                qWarning() << "Could not reload virtual mod group store after update";
            } else {
                reloadedGroupState = true;
            }
        } else if (!m_groupStore->loadOrCreate()) {
            qWarning() << "Could not initialize virtual mod group store after update";
        } else {
            reloadedGroupState = true;
        }

        if (reloadedGroupState) {
            emit virtualGroupsChanged();
        }
    }

    syncVirtualGroupsFromResources();
}

void ModFolderModel::onParseSucceeded(int ticket, QString mod_id)
{
    auto iter = m_active_parse_tasks.constFind(ticket);
    if (iter == m_active_parse_tasks.constEnd())
        return;

    int row = m_resources_index[mod_id];

    auto parse_task = *iter;
    auto cast_task = static_cast<LocalModParseTask*>(parse_task.get());

    Q_ASSERT(cast_task->token() == ticket);

    auto resource = find(mod_id);

    auto result = cast_task->result();
    if (result && resource)
        static_cast<Mod*>(resource.get())->finishResolvingWithDetails(std::move(result->details));

    emit dataChanged(index(row, RequiresColumn), index(row, RequiredByColumn));
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
            int row = m_resources_index[mod->internal_id()];
            emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));
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
        auto affectedId = affected->mod_id();
        auto row = m_resources_index[affected->internal_id()];
        affectedList << index(row, 0);
    }
    return affectedList;
}

bool ModFolderModel::setResourceEnabled(const QModelIndexList& indexes, EnableAction action)
{
    if (indexes.isEmpty())
        return {};

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
    auto toList = [this](QSet<Mod*> mods) {
        QModelIndexList list;
        for (auto mod : mods) {
            auto row = m_resources_index[mod->internal_id()];
            list << index(row, 0);
        }
        return list;
    };

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

    auto disableStatus = ResourceFolderModel::setResourceEnabled(toList(toDisable), EnableAction::DISABLE);
    auto enableStatus = ResourceFolderModel::setResourceEnabled(toList(toEnable), EnableAction::ENABLE);
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
    auto rsp = ResourceFolderModel::deleteResources(indexes);
    for (auto mod : allMods()) {
        auto id = mod->mod_id();
        deleteInvalid(m_requiredBy[id]);
        deleteInvalid(m_requires[id]);
        if (mod->requiredByCount() != m_requiredBy[id].count() || mod->requiresCount() != m_requires[id].count()) {
            mod->setRequiredByCount(m_requiredBy[id].count());
            mod->setRequiresCount(m_requires[id].count());
            int row = m_resources_index[mod->internal_id()];
            emit dataChanged(index(row, RequiresColumn), index(row, RequiredByColumn));
        }
    }
    return rsp;
}

bool ModFolderModel::createGroup(const QString& name)
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return false;
    }

    auto groupId = m_groupStore->createGroup(name);
    if (groupId.isEmpty()) {
        return false;
    }

    if (!m_groupStore->save()) {
        return false;
    }

    emit virtualGroupsChanged();
    return true;
}

bool ModFolderModel::renameGroup(const QString& groupId, const QString& newName)
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return false;
    }

    if (!m_groupStore->renameGroup(groupId, newName)) {
        return false;
    }

    if (!m_groupStore->save()) {
        return false;
    }

    emit virtualGroupsChanged();
    return true;
}

bool ModFolderModel::deleteGroup(const QString& groupId)
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr || groupId.isEmpty()) {
        return false;
    }

    if (!m_groupStore->deleteGroup(groupId)) {
        return false;
    }

    if (!m_groupStore->save()) {
        return false;
    }

    if (m_activeGroupId == groupId) {
        m_activeGroupId.clear();
        emit activeGroupChanged(m_activeGroupId);
    }

    emit virtualGroupsChanged();
    return true;
}

bool ModFolderModel::assignModsToGroup(const QStringList& fileKeys, const QString& groupId)
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return false;
    }

    if (!m_groupStore->assignEntriesToGroup(fileKeys, groupId)) {
        return false;
    }

    if (!m_groupStore->save()) {
        return false;
    }

    emit virtualGroupsChanged();
    return true;
}

bool ModFolderModel::setActiveGroup(const QString& groupIdOrAll)
{
    QString normalizedGroupId = groupIdOrAll;
    if (normalizedGroupId.compare("all", Qt::CaseInsensitive) == 0) {
        normalizedGroupId.clear();
    }

    if (normalizedGroupId == m_activeGroupId) {
        return true;
    }

    if (!normalizedGroupId.isEmpty() &&
        (!m_virtualGroupsEnabled || m_groupStore == nullptr || !m_groupStore->groupExists(normalizedGroupId))) {
        return false;
    }

    m_activeGroupId = normalizedGroupId;
    emit activeGroupChanged(m_activeGroupId);
    return true;
}

QList<Resource*> ModFolderModel::modsForActiveGroupSelection()
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr || m_activeGroupId.isEmpty()) {
        return allResources();
    }

    QList<Resource*> resources;
    for (auto const& resource : m_resources) {
        if (isResourceInActiveGroup(*resource)) {
            resources.push_back(resource.get());
        }
    }
    return resources;
}

QList<ModFolderModel::GroupOption> ModFolderModel::groupOptions() const
{
    QList<GroupOption> options;
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return options;
    }

    auto groups = m_groupStore->groups();
    std::sort(groups.begin(), groups.end(), [](const VirtualModGroupStore::Group& left, const VirtualModGroupStore::Group& right) {
        return left.name.localeAwareCompare(right.name) < 0;
    });
    options.reserve(groups.size());
    for (auto const& group : groups) {
        GroupOption option;
        option.id = group.id;
        option.label = group.name;
        option.managedPack = group.kind == VirtualModGroupStore::GroupKind::MANAGED_PACK;
        options.push_back(option);
    }

    return options;
}

QStringList ModFolderModel::fileKeysForIndexes(const QModelIndexList& indexes) const
{
    QStringList fileKeys;
    for (auto const& index : indexes) {
        if (index.column() != 0 || !validateIndex(index)) {
            continue;
        }

        auto const& resource = at(index.row());
        if (resource.type() == ResourceType::FOLDER) {
            continue;
        }

        fileKeys.append(fileKeyForResource(resource));
    }
    fileKeys.removeDuplicates();
    return fileKeys;
}

QString ModFolderModel::groupForFileKey(const QString& fileKey) const
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return {};
    }

    auto entry = m_groupStore->entry(fileKey);
    if (!entry.has_value()) {
        return {};
    }

    return entry->groupId;
}

bool ModFolderModel::isManagedGroup(const QString& groupId) const
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr || groupId.isEmpty()) {
        return false;
    }

    auto groups = m_groupStore->groups();
    auto matchIter = std::find_if(groups.cbegin(), groups.cend(), [&groupId](const auto& group) { return group.id == groupId; });
    if (matchIter == groups.cend()) {
        return false;
    }

    return matchIter->kind == VirtualModGroupStore::GroupKind::MANAGED_PACK;
}

void ModFolderModel::syncVirtualEntry(Resource* resource)
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr || resource == nullptr || resource->type() == ResourceType::FOLDER) {
        return;
    }

    auto fileKey = fileKeyForResource(*resource);
    auto existingEntry = m_groupStore->entry(fileKey);

    VirtualModGroupStore::Entry entry;
    if (existingEntry.has_value()) {
        entry = existingEntry.value();
    }
    entry.fileKey = fileKey;
    entry.fileName = resource->fileinfo().fileName();

    if (resource->metadata()) {
        entry.linkedMetadataSlug = resource->metadata()->slug;
        entry.provider = QString::fromUtf8(ModPlatform::ProviderCapabilities::name(resource->metadata()->provider));
        entry.projectId = resource->metadata()->project_id.toString();

        if (entry.sourceType != VirtualModGroupStore::SourceType::MANAGED_PACK) {
            entry.sourceType = VirtualModGroupStore::SourceType::PROVIDER_LINKED;
        }
    } else if (entry.sourceType != VirtualModGroupStore::SourceType::MANAGED_PACK) {
        entry.sourceType = VirtualModGroupStore::SourceType::LOCAL_NO_SOURCE;
        entry.linkedMetadataSlug.clear();
        entry.provider.clear();
        entry.projectId.clear();
    }

    m_groupStore->upsertEntry(std::move(entry));
}

void ModFolderModel::updateManagedPackOwnership(const QStringList& fileNames,
                                                const QString& managedPackType,
                                                const QString& managedPackId,
                                                const QString& managedPackName)
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return;
    }

    auto normalizedManagedPackId = managedPackId.trimmed().isEmpty() ? managedPackName.trimmed() : managedPackId.trimmed();
    auto groupId = m_groupStore->findManagedPackGroup(managedPackType, normalizedManagedPackId);
    if (groupId.isEmpty()) {
        return;
    }

    for (auto fileName : fileNames) {
        fileName = QFileInfo(fileName).fileName();
        if (fileName.isEmpty()) {
            continue;
        }

        auto fileKey = VirtualModGroupStore::fileKeyForFileName(fileName);
        auto existingEntry = m_groupStore->entry(fileKey);

        VirtualModGroupStore::Entry entry;
        if (existingEntry.has_value()) {
            entry = existingEntry.value();
        }

        entry.fileKey = fileKey;
        entry.fileName = fileName;
        entry.groupId = groupId;
        entry.sourceType = VirtualModGroupStore::SourceType::MANAGED_PACK;

        m_groupStore->upsertEntry(std::move(entry));
    }

    if (m_groupStore->save()) {
        emit virtualGroupsChanged();
    }
}

bool ModFolderModel::shouldTreatFileAsManagedPackOwned(const QString& fileName,
                                                       const QString& managedPackType,
                                                       const QString& managedPackId) const
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return true;
    }

    auto normalizedManagedPackId = managedPackId.trimmed();
    if (normalizedManagedPackId.isEmpty() && m_instance != nullptr) {
        normalizedManagedPackId = m_instance->getManagedPackName();
    }
    auto managedGroupId = m_groupStore->findManagedPackGroup(managedPackType, normalizedManagedPackId);
    if (managedGroupId.isEmpty()) {
        return true;
    }

    auto fileKey = VirtualModGroupStore::fileKeyForFileName(QFileInfo(fileName).fileName());
    auto entryOpt = m_groupStore->entry(fileKey);
    if (!entryOpt.has_value()) {
        return true;
    }

    if (entryOpt->groupId.isEmpty()) {
        return false;
    }

    return entryOpt->groupId == managedGroupId;
}

bool ModFolderModel::isResourceInActiveGroup(const Resource& resource) const
{
    if (m_activeGroupId.isEmpty() || !m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return true;
    }

    auto fileKey = fileKeyForResource(resource);
    return m_groupStore->isEntryInGroup(fileKey, m_activeGroupId);
}

QString ModFolderModel::fileKeyForResource(const Resource& resource) const
{
    return VirtualModGroupStore::fileKeyForFileName(resource.getOriginalFileName());
}

void ModFolderModel::initializeVirtualGroups()
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return;
    }

    if (!m_groupStore->exists()) {
        if (!m_groupStore->loadOrCreate()) {
            qWarning() << "Could not initialize virtual mod group store";
            return;
        }
        bootstrapVirtualGroupsFromCurrentState();
        classifyManagedPackEntriesFromManifests();
        if (!m_groupStore->save()) {
            qWarning() << "Could not save virtual mod group store during initialization";
        }
        return;
    }

    if (!m_groupStore->load()) {
        qWarning() << "Could not parse existing virtual mod group store; recreating";
        if (!m_groupStore->loadOrCreate()) {
            qWarning() << "Could not recreate virtual mod group store";
            return;
        }
        bootstrapVirtualGroupsFromCurrentState();
        classifyManagedPackEntriesFromManifests();
        if (!m_groupStore->save()) {
            qWarning() << "Could not save recreated virtual mod group store";
        }
    }
}

void ModFolderModel::bootstrapVirtualGroupsFromCurrentState()
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return;
    }

    QSet<QString> existingFileKeys;
    for (auto const& fileInfo : m_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable)) {
        auto fileName = fileInfo.fileName();
        auto fileKey = VirtualModGroupStore::fileKeyForFileName(fileName);
        existingFileKeys.insert(fileKey);

        auto existingEntry = m_groupStore->entry(fileKey);
        VirtualModGroupStore::Entry entry;
        if (existingEntry.has_value()) {
            entry = existingEntry.value();
        }

        entry.fileKey = fileKey;
        entry.fileName = fileName;

        auto metadata = Metadata::get(indexDir(), fileKey);
        if (metadata.isValid()) {
            entry.sourceType = VirtualModGroupStore::SourceType::PROVIDER_LINKED;
            entry.linkedMetadataSlug = metadata.slug;
            entry.provider = QString::fromUtf8(ModPlatform::ProviderCapabilities::name(metadata.provider));
            entry.projectId = metadata.project_id.toString();
        } else if (entry.sourceType != VirtualModGroupStore::SourceType::MANAGED_PACK) {
            entry.sourceType = VirtualModGroupStore::SourceType::LOCAL_NO_SOURCE;
            entry.linkedMetadataSlug.clear();
            entry.provider.clear();
            entry.projectId.clear();
        }

        m_groupStore->upsertEntry(std::move(entry));
    }

    m_groupStore->removeEntriesNotIn(existingFileKeys);
}

bool ModFolderModel::classifyManagedPackEntriesFromManifests()
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr || m_instance == nullptr || !m_instance->isManagedPack()) {
        return false;
    }

    auto managedPackType = m_instance->getManagedPackType();
    if (managedPackType.isEmpty()) {
        return false;
    }

    auto managedPackId = m_instance->getManagedPackID();
    if (managedPackId.isEmpty()) {
        managedPackId = m_instance->getManagedPackName();
    }

    bool changed = false;
    auto managedGroupId = m_groupStore->findManagedPackGroup(managedPackType, managedPackId);
    if (managedGroupId.isEmpty()) {
        return false;
    }

    if (managedPackType == "modrinth") {
        auto manifestPath = FS::PathCombine(m_instance->instanceRoot(), "mrpack", "modrinth.index.json");
        auto managedFileKeys = parseModrinthManagedFileKeys(manifestPath);
        for (auto const& fileKey : managedFileKeys) {
            auto entryOpt = m_groupStore->entry(fileKey);
            if (!entryOpt.has_value()) {
                continue;
            }

            auto entry = entryOpt.value();
            if (entry.groupId != managedGroupId || entry.sourceType != VirtualModGroupStore::SourceType::MANAGED_PACK) {
                changed = true;
            }
            entry.groupId = managedGroupId;
            entry.sourceType = VirtualModGroupStore::SourceType::MANAGED_PACK;
            m_groupStore->upsertEntry(std::move(entry));
        }
    } else if (managedPackType == "flame") {
        auto manifestPath = FS::PathCombine(m_instance->instanceRoot(), "flame", "manifest.json");
        auto managedProjectIds = parseFlameManagedProjectIds(manifestPath);

        auto currentEntries = m_groupStore->entries();
        for (auto& entry : currentEntries) {
            if (entry.provider.compare("curseforge", Qt::CaseInsensitive) != 0 &&
                entry.provider.compare("flame", Qt::CaseInsensitive) != 0) {
                continue;
            }
            if (!managedProjectIds.contains(entry.projectId)) {
                continue;
            }

            if (entry.groupId != managedGroupId || entry.sourceType != VirtualModGroupStore::SourceType::MANAGED_PACK) {
                changed = true;
            }
            entry.groupId = managedGroupId;
            entry.sourceType = VirtualModGroupStore::SourceType::MANAGED_PACK;
            m_groupStore->upsertEntry(std::move(entry));
        }
    }

    return changed;
}

void ModFolderModel::syncVirtualGroupsFromResources()
{
    if (!m_virtualGroupsEnabled || m_groupStore == nullptr) {
        return;
    }

    bool storeMissingOnDisk = !m_groupStore->exists();

    auto beforeEntries = virtualEntryMapByKey(m_groupStore->entries());
    QHash<QString, GroupCarryHint> projectHints;
    QHash<QString, GroupCarryHint> slugHints;
    buildGroupCarryHints(beforeEntries, projectHints, slugHints);

    QSet<QString> fileKeys;
    for (auto const& resource : m_resources) {
        if (resource->type() == ResourceType::FOLDER) {
            continue;
        }

        syncVirtualEntry(resource.get());
        auto fileKey = fileKeyForResource(*resource);
        fileKeys.insert(fileKey);

        if (beforeEntries.contains(fileKey)) {
            continue;
        }

        auto entryOpt = m_groupStore->entry(fileKey);
        if (!entryOpt.has_value() || !entryOpt->groupId.isEmpty()) {
            continue;
        }

        GroupCarryHint hint;
        bool hasHint = false;

        auto projectIdentity = groupCarryProjectIdentity(*entryOpt);
        if (!projectIdentity.isEmpty()) {
            auto projectHintIt = projectHints.constFind(projectIdentity);
            if (projectHintIt != projectHints.constEnd()) {
                hint = projectHintIt.value();
                hasHint = true;
            }
        }

        if (!hasHint) {
            auto slugIdentity = groupCarrySlugIdentity(*entryOpt);
            if (!slugIdentity.isEmpty()) {
                auto slugHintIt = slugHints.constFind(slugIdentity);
                if (slugHintIt != slugHints.constEnd()) {
                    hint = slugHintIt.value();
                    hasHint = true;
                }
            }
        }

        if (!hasHint || hint.groupId.isEmpty()) {
            continue;
        }

        auto entry = entryOpt.value();
        entry.groupId = hint.groupId;
        if (hint.sourceType == VirtualModGroupStore::SourceType::MANAGED_PACK) {
            entry.sourceType = VirtualModGroupStore::SourceType::MANAGED_PACK;
        }
        m_groupStore->upsertEntry(std::move(entry));
    }

    m_groupStore->removeEntriesNotIn(fileKeys);
    auto managedClassificationChanged = classifyManagedPackEntriesFromManifests();

    auto afterEntries = virtualEntryMapByKey(m_groupStore->entries());
    bool entriesChanged = !areSameVirtualEntryMaps(beforeEntries, afterEntries);

    bool activeGroupDidChange = false;
    if (!m_activeGroupId.isEmpty() && !m_groupStore->groupExists(m_activeGroupId)) {
        m_activeGroupId.clear();
        emit activeGroupChanged(m_activeGroupId);
        activeGroupDidChange = true;
    }

    if (!entriesChanged && !activeGroupDidChange && !storeMissingOnDisk && !managedClassificationChanged) {
        return;
    }

    if (!m_groupStore->save()) {
        qWarning() << "Could not save virtual mod group store after sync";
    }

    if (entriesChanged || activeGroupDidChange) {
        emit virtualGroupsChanged();
    }
}

bool ModFolderModel::ProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (!ResourceFolderModel::ProxyModel::filterAcceptsRow(source_row, source_parent)) {
        return false;
    }

    auto* model = qobject_cast<ModFolderModel*>(sourceModel());
    if (model == nullptr) {
        return true;
    }

    if (!model->virtualGroupsEnabled() || model->activeGroup().isEmpty()) {
        return true;
    }

    if (source_row < 0 || source_row >= model->rowCount()) {
        return false;
    }

    return model->isResourceInActiveGroup(model->at(source_row));
}
