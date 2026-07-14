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
#include <QUuid>
#include <algorithm>
#include <set>

#include "minecraft/mod/Resource.h"
#include "minecraft/mod/ResourceFolderModel.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "modplatform/ModIndex.h"
#include "ui/dialogs/CustomMessageBox.h"

ModFolderModel::ModFolderModel(const QDir& dir, BaseInstance* instance, bool isIndexed, bool createDir, QObject* parent)
    : ResourceFolderModel(QDir(dir), instance, isIndexed, createDir, parent)
{
    m_columnNames = QStringList({ "Enable", "Image", "Name", "Version", "Last Modified", "Provider", "Size", "Side", "Loaders",
                                  "Minecraft Versions", "Release Type", "Requires", "Required By", "File Name" });
    m_columnNamesTranslated =
        QStringList({ tr("Enable"), tr("Image"), tr("Name"), tr("Version"), tr("Last Modified"), tr("Provider"), tr("Size"), tr("Side"),
                      tr("Loaders"), tr("Minecraft Versions"), tr("Release Type"), tr("Requires"), tr("Required By"), tr("File Name") });
    m_columnSortKeys = { SortType::Enabled,     SortType::Name,     SortType::Name,       SortType::Version, SortType::Date,
                         SortType::Provider,    SortType::Size,     SortType::Side,       SortType::Loaders, SortType::McVersions,
                         SortType::ReleaseType, SortType::Requires, SortType::RequiredBy, SortType::Filename };
    m_columnResizeModes = { QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Stretch,     QHeaderView::Interactive,
                            QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive,
                            QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive,
                            QHeaderView::Interactive, QHeaderView::Interactive };
    m_columnsHideable = { false, true, false, true, true, true, true, true, true, true, true, true, true, true };

    connect(this, &ModFolderModel::parseFinished, this, &ModFolderModel::onParseFinished);
}

QVariant ModFolderModel::data(const QModelIndex& index, int role) const
{
    if (!validateIndex(index)) {
        return {};
    }

    int row = index.row();
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
                    return at(row).mcVersionsString();
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
                default:
                    break;
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

    // map the columns to the base equivilents
    QModelIndex mappedIndex;
    switch (column) {
        case ActiveColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::ActiveColumn);
            break;
        case NameColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::NameColumn);
            break;
        case DateColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::DateColumn);
            break;
        case ProviderColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::ProviderColumn);
            break;
        case SizeColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::SizeColumn);
            break;
        case FileNameColumn:
            mappedIndex = index.siblingAtColumn(ResourceFolderModel::FileNameColumn);
            break;
        default:
            break;
    }

    if (mappedIndex.isValid()) {
        return ResourceFolderModel::data(mappedIndex, role);
    }

    return {};
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
                case FileNameColumn:
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
                case FileNameColumn:
                    return tr("The file name of the mod.");
                default:
                    return QVariant();
            }
        default:
            return QVariant();
    }
}

int ModFolderModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : NumColumns;
}

Task* ModFolderModel::createParseTask(Resource& resource)
{
    return new LocalModParseTask(m_nextResolutionTicket, resource.type(), resource.fileinfo());
}

bool ModFolderModel::isValid()
{
    return m_dir.exists() && m_dir.isReadable();
}

void ModFolderModel::onParseSucceeded(int ticket, const QString& resourceId)
{
    auto iter = m_activeParseTasks.constFind(ticket);
    if (iter == m_activeParseTasks.constEnd()) {
        return;
    }

    int row = m_resourcesIndex[resourceId];

    const auto& parseTask = *iter;
    auto* castTask = static_cast<LocalModParseTask*>(parseTask.get());

    Q_ASSERT(castTask->token() == ticket);

    auto resource = find(resourceId);

    auto result = castTask->result();
    if (result && resource) {
        auto* mod = static_cast<Mod*>(resource.get());
        mod->finishResolvingWithDetails(std::move(result->details));
    }
    emit dataChanged(index(row, RequiresColumn), index(row, RequiredByColumn));
}

namespace {
Mod* findById(QSet<Mod*> mods, const QString& resourceId)
{
    auto found = std::ranges::find_if(mods, [resourceId](Mod* m) { return m->mod_id() == resourceId; });
    return found != mods.end() ? *found : nullptr;
}
}  // namespace

void ModFolderModel::onParseFinished()
{
    if (hasPendingParseTasks()) {
        return;
    }
    auto modsList = allMods();
    auto mods = QSet(modsList.begin(), modsList.end());

    m_requires.clear();
    m_requiredBy.clear();

    auto findByProjectID = [mods](const QVariant& modId, ModPlatform::ResourceProvider provider) -> Mod* {
        auto found = std::ranges::find_if(mods, [modId, provider](Mod* m) {
            return m->metadata() && m->metadata()->provider == provider && m->metadata()->project_id == modId;
        });
        return found != mods.end() ? *found : nullptr;
    };
    for (auto* mod : mods) {
        auto id = mod->mod_id();
        for (const auto& dep : mod->dependencies()) {
            auto* d = findById(mods, dep);
            if (d) {
                m_requires[id] << d;
                m_requiredBy[d->mod_id()] << mod;
            }
        }
        if (mod->metadata()) {
            for (const auto& dep : mod->metadata()->dependencies) {
                if (dep.type == ModPlatform::DependencyType::REQUIRED) {
                    auto* d = findByProjectID(dep.addonId, mod->metadata()->provider);
                    if (d) {
                        m_requires[id] << d;
                        m_requiredBy[d->mod_id()] << mod;
                    }
                }
            }
        }
    }
    for (auto* mod : mods) {
        auto id = mod->mod_id();
        if (mod->requiredByCount() != m_requiredBy[id].count() || mod->requiresCount() != m_requires[id].count()) {
            mod->setRequiredByCount(m_requiredBy[id].count());
            mod->setRequiresCount(m_requires[id].count());
            int row = m_resourcesIndex[mod->internalId()];
            emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));
        }
    }
}

namespace {

QSet<Mod*> collectMods(const QSet<Mod*>& mods, QHash<QString, QSet<Mod*>> relation, std::set<QString>& seen, bool shouldBeEnabled)
{
    QSet<Mod*> affectedList = {};
    QSet<Mod*> needToCheck = {};
    for (auto* mod : mods) {
        auto id = mod->mod_id();
        if (!seen.contains(id)) {
            seen.insert(id);
            for (auto* affected : relation[id]) {
                auto affectedId = affected->mod_id();

                if (findById(mods, affectedId) == nullptr && !seen.contains(affectedId)) {
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
}  // namespace

QModelIndexList ModFolderModel::getAffectedMods(const QModelIndexList& indexes, EnableAction action)
{
    if (indexes.isEmpty()) {
        return {};
    }

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
    for (auto* affected : affectedMods) {
        auto affectedId = affected->mod_id();
        auto row = m_resourcesIndex[affected->internalId()];
        affectedList << index(row, 0);
    }
    return affectedList;
}

bool ModFolderModel::setResourceEnabled(const QModelIndexList& indexes, EnableAction action)
{
    if (indexes.isEmpty()) {
        return {};
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
            for (auto* mod : indexedMods) {
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
    auto toList = [this](const QSet<Mod*>& mods) {
        QModelIndexList list;
        for (auto* mod : mods) {
            auto row = m_resourcesIndex[mod->internalId()];
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

        auto* box = CustomMessageBox::selectable(nullptr, title, message, QMessageBox::Warning,
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

namespace {
QStringList reqToList(const QSet<Mod*>& l)
{
    QStringList req;
    for (auto* m : l) {
        req << m->name();
    }
    return req;
}
}  // namespace

QStringList ModFolderModel::requiresList(const QString& id)
{
    return reqToList(m_requires[id]);
}

QStringList ModFolderModel::requiredByList(const QString& id)
{
    return reqToList(m_requiredBy[id]);
}

bool ModFolderModel::deleteResources(const QModelIndexList& indexes)
{
    auto deleteInvalid = [](QSet<Mod*>& mods) {
        for (auto it = mods.begin(); it != mods.end();) {
            auto* mod = *it;
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
    for (auto* mod : allMods()) {
        auto id = mod->mod_id();
        deleteInvalid(m_requiredBy[id]);
        deleteInvalid(m_requires[id]);
        if (mod->requiredByCount() != m_requiredBy[id].count() || mod->requiresCount() != m_requires[id].count()) {
            mod->setRequiredByCount(m_requiredBy[id].count());
            mod->setRequiresCount(m_requires[id].count());
            int row = m_resourcesIndex[mod->internalId()];
            emit dataChanged(index(row, RequiresColumn), index(row, RequiredByColumn));
        }
    }
    return rsp;
}
