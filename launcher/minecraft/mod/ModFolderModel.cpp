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

#include "minecraft/Component.h"
#include "minecraft/mod/Resource.h"
#include "minecraft/mod/ResourceFolderModel.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "modplatform/ModIndex.h"
#include "ui/dialogs/CustomMessageBox.h"

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

    connect(this, &ModFolderModel::parseFinished, this, &ModFolderModel::onParseFinished);
}

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

Task* ModFolderModel::createParseTask(Resource& resource)
{
    return new LocalModParseTask(m_next_resolution_ticket, resource.type(), resource.fileinfo());
}

bool ModFolderModel::isValid()
{
    return m_dir.exists() && m_dir.isReadable();
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
