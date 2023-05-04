// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
*  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
#include <QDebug>
#include <QFileSystemWatcher>
#include <QIcon>
#include <QMimeData>
#include <QString>
#include <QStyle>
#include <QThreadPool>
#include <QUrl>
#include <QUuid>
#include <algorithm>

#include "Application.h"

#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "minecraft/mod/tasks/ModFolderLoadTask.h"
#include "modplatform/ModIndex.h"

ModFolderModel::ModFolderModel(const QString& dir, std::shared_ptr<const BaseInstance> instance, bool is_indexed, bool create_dir)
    : ResourceFolderModel(QDir(dir), instance, nullptr, create_dir), m_is_indexed(is_indexed)
{
    m_column_sort_keys = { SortType::ENABLED, SortType::NAME, SortType::VERSION, SortType::DATE, SortType::PROVIDER };
}

QVariant ModFolderModel::data(const QModelIndex &index, int role) const
{
    if (!validateIndex(index))
        return {};

    int row = index.row();
    int column = index.column();

    switch (role)
    {
    case Qt::DisplayRole:
        switch (column)
        {
        case NameColumn:
            return m_resources[row]->name();
        case VersionColumn: {
            switch(m_resources[row]->type()) {
                case ResourceType::FOLDER:
                    return tr("Folder");
                case ResourceType::SINGLEFILE:
                    return tr("File");
                default:
                    break;
            }
            return at(row)->version();
        }
        case DateColumn:
            return m_resources[row]->dateTimeChanged();
        case ProviderColumn: {
            auto provider = at(row)->provider();
            if (!provider.has_value()) {
	            //: Unknown mod provider (i.e. not Modrinth, CurseForge, etc...)
                return tr("Unknown");
            }

            return provider.value();
        }
        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        if (column == NAME_COLUMN) {
            if (at(row)->isSymLinkUnder(instDirPath())) {
                return m_resources[row]->internal_id() +
                    tr("\nWarning: This resource is symbolically linked from elsewhere. Editing it will also change the original." 
                       "\nCanonical Path: %1")
                        .arg(at(row)->fileinfo().canonicalFilePath());
            }
            if (at(row)->isMoreThanOneHardLink()) {
                return m_resources[row]->internal_id() +
                    tr("\nWarning: This resource is hard linked elsewhere. Editing it will also change the original.");
            }
        }
        return m_resources[row]->internal_id();
    case Qt::DecorationRole: {
        if (column == NAME_COLUMN && (at(row)->isSymLinkUnder(instDirPath()) || at(row)->isMoreThanOneHardLink()))
            return APPLICATION->getThemedIcon("status-yellow");

        return {};
    }
    case Qt::CheckStateRole:
        switch (column)
        {
        case ActiveColumn:
            return at(row)->enabled() ? Qt::Checked : Qt::Unchecked;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QVariant ModFolderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
        switch (section)
        {
        case ActiveColumn:
            return QString();
        case NameColumn:
            return tr("Name");
        case VersionColumn:
            return tr("Version");
        case DateColumn:
            return tr("Last changed");
        case ProviderColumn:
            return tr("Provider");
        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        switch (section)
        {
        case ActiveColumn:
            return tr("Is the mod enabled?");
        case NameColumn:
            return tr("The name of the mod.");
        case VersionColumn:
            return tr("The version of the mod.");
        case DateColumn:
            return tr("The date and time this mod was last changed (or added).");
        case ProviderColumn:
            return tr("Where the mod was downloaded from.");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
    return QVariant();
}

int ModFolderModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NUM_COLUMNS;
}

Task* ModFolderModel::createUpdateTask()
{
    auto index_dir = indexDir();
    auto task = new ModFolderLoadTask(dir(), index_dir, m_is_indexed, m_first_folder_load);
    m_first_folder_load = false;
    return task;
}

Task* ModFolderModel::createParseTask(Resource& resource)
{
    return new LocalModParseTask(m_next_resolution_ticket, resource.type(), resource.fileinfo());
}

bool ModFolderModel::uninstallMod(const QString& filename, bool preserve_metadata)
{
    for(auto mod : allMods()){
        if(mod->fileinfo().fileName() == filename){
            auto index_dir = indexDir();
            mod->destroy(index_dir, preserve_metadata);

            update();

            return true;
        }
    }

    return false;
}

bool ModFolderModel::deleteMods(const QModelIndexList& indexes)
{
    if(!m_can_interact) {
        return false;
    }

    if(indexes.isEmpty())
        return true;

    for (auto i: indexes)
    {
        if(i.column() != 0) {
            continue;
        }
        auto m = at(i.row());
        auto index_dir = indexDir();
        m->destroy(index_dir);
    }

    update();

    return true;
}

bool ModFolderModel::isValid()
{
    return m_dir.exists() && m_dir.isReadable();
}

bool ModFolderModel::startWatching()
{
    // Remove orphaned metadata next time
    m_first_folder_load = true;
    return ResourceFolderModel::startWatching({ m_dir.absolutePath(), indexDir().absolutePath() });
}

bool ModFolderModel::stopWatching()
{
    return ResourceFolderModel::stopWatching({ m_dir.absolutePath(), indexDir().absolutePath() });
}

auto ModFolderModel::selectedMods(QModelIndexList& indexes) -> QList<Mod*>
{
    QList<Mod*> selected_resources;
    for (auto i : indexes) {
        if(i.column() != 0)
            continue;

        selected_resources.push_back(at(i.row()));
    }
    return selected_resources;
}

auto ModFolderModel::allMods() -> QList<Mod*>
{
    QList<Mod*> mods;

    for (auto& res : qAsConst(m_resources)) {
        mods.append(static_cast<Mod*>(res.get()));
    }

    return mods;
}

void ModFolderModel::onUpdateSucceeded()
{
    auto update_results = static_cast<ModFolderLoadTask*>(m_current_update_task.get())->result();

    auto& new_mods = update_results->mods;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto current_list = m_resources_index.keys();
    QSet<QString> current_set(current_list.begin(), current_list.end());

    auto new_list = new_mods.keys();
    QSet<QString> new_set(new_list.begin(), new_list.end());
#else
    QSet<QString> current_set(m_resources_index.keys().toSet());
    QSet<QString> new_set(new_mods.keys().toSet());
#endif

    applyUpdates(current_set, new_set, new_mods);
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
        resource->finishResolvingWithDetails(std::move(result->details));

    emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));
}
