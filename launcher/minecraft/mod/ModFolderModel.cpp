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
#include <QMimeData>
#include <QString>
#include <QThreadPool>
#include <QUrl>
#include <QUuid>
#include <algorithm>

#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "minecraft/mod/tasks/ModFolderLoadTask.h"

ModFolderModel::ModFolderModel(const QString &dir, bool is_indexed) : ResourceFolderModel(dir), m_is_indexed(is_indexed)
{
    FS::ensureFolderPathExists(m_dir.absolutePath());
    m_column_sort_keys = { SortType::ENABLED, SortType::NAME, SortType::VERSION, SortType::DATE };
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

        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        return m_resources[row]->internal_id();

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

bool ModFolderModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
    {
        return false;
    }

    if (role == Qt::CheckStateRole)
    {
        return setModStatus(index.row(), Toggle);
    }
    return false;
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
    return NUM_COLUMNS;
}

Task* ModFolderModel::createUpdateTask()
{
    auto index_dir = indexDir();
    auto task = new ModFolderLoadTask(dir(), index_dir, m_is_indexed, m_first_folder_load);
    m_first_folder_load = false;
    return task;
}

Task* ModFolderModel::createParseTask(Resource const& resource)
{
    return new LocalModParseTask(m_next_resolution_ticket, resource.type(), resource.fileinfo());
}

bool ModFolderModel::uninstallMod(const QString& filename, bool preserve_metadata)
{
    for(auto mod : allMods()){
        if(mod->fileinfo().fileName() == filename){
            auto index_dir = indexDir();
            mod->destroy(index_dir, preserve_metadata);
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

auto ModFolderModel::selectedMods(QModelIndexList& indexes) -> QList<Mod::Ptr>
{
    QList<Mod::Ptr> selected_resources;
    for (auto i : indexes) {
        if(i.column() != 0)
            continue;

        selected_resources.push_back(at(i.row()));
    }
    return selected_resources;
}

auto ModFolderModel::allMods() -> QList<Mod::Ptr>
{
    QList<Mod::Ptr> mods;

    for (auto res : m_resources)
        mods.append(static_cast<Mod*>(res.get()));

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
    
    update_results.reset();
    m_current_update_task.reset();

    emit updateFinished();

    if(m_scheduled_update) {
        m_scheduled_update = false;
        update();
    }
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
        resource->finishResolvingWithDetails(result->details);

    emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));

    parse_task->deleteLater();
    m_active_parse_tasks.remove(ticket);
}


bool ModFolderModel::setModStatus(const QModelIndexList& indexes, ModStatusAction enable)
{
    if(!m_can_interact) {
        return false;
    }

    if(indexes.isEmpty())
        return true;

    for (auto index: indexes)
    {
        if(index.column() != 0) {
            continue;
        }
        setModStatus(index.row(), enable);
    }
    return true;
}

bool ModFolderModel::setModStatus(int row, ModFolderModel::ModStatusAction action)
{
    if(row < 0 || row >= m_resources.size()) {
        return false;
    }

    auto mod = at(row);
    bool desiredStatus;
    switch(action) {
        case Enable:
            desiredStatus = true;
            break;
        case Disable:
            desiredStatus = false;
            break;
        case Toggle:
        default:
            desiredStatus = !mod->enabled();
            break;
    }

    if(desiredStatus == mod->enabled()) {
        return true;
    }

    // preserve the row, but change its ID
    auto oldId = mod->internal_id();
    if(!mod->enable(!mod->enabled())) {
        return false;
    }
    auto newId = mod->internal_id();
    if(m_resources_index.contains(newId)) {
        // NOTE: this could handle a corner case, where we are overwriting a file, because the same 'mod' exists both enabled and disabled
        // But is it necessary?
    }
    m_resources_index.remove(oldId);
    m_resources_index[newId] = row;
    emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex()) - 1));
    return true;
}

