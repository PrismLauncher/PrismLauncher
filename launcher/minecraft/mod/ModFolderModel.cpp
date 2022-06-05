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

ModFolderModel::ModFolderModel(const QString &dir, bool is_indexed) : QAbstractListModel(), m_dir(dir), m_is_indexed(is_indexed)
{
    FS::ensureFolderPathExists(m_dir.absolutePath());
    m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    m_dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(directoryChanged(QString)));
}

void ModFolderModel::startWatching()
{
    if(is_watching)
        return;

    update();

    is_watching = m_watcher->addPath(m_dir.absolutePath());
    if (is_watching)
    {
        qDebug() << "Started watching " << m_dir.absolutePath();
    }
    else
    {
        qDebug() << "Failed to start watching " << m_dir.absolutePath();
    }
}

void ModFolderModel::stopWatching()
{
    if(!is_watching)
        return;

    is_watching = !m_watcher->removePath(m_dir.absolutePath());
    if (!is_watching)
    {
        qDebug() << "Stopped watching " << m_dir.absolutePath();
    }
    else
    {
        qDebug() << "Failed to stop watching " << m_dir.absolutePath();
    }
}

bool ModFolderModel::update()
{
    if (!isValid()) {
        return false;
    }
    if(m_update) {
        scheduled_update = true;
        return true;
    }

    auto index_dir = indexDir();
    auto task = new ModFolderLoadTask(dir(), index_dir, m_is_indexed);

    m_update = task->result();

    QThreadPool *threadPool = QThreadPool::globalInstance();
    connect(task, &ModFolderLoadTask::succeeded, this, &ModFolderModel::finishUpdate);
    
    threadPool->start(task);
    return true;
}

void ModFolderModel::finishUpdate()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto currentList = modsIndex.keys();
    QSet<QString> currentSet(currentList.begin(), currentList.end());
    auto & newMods = m_update->mods;
    auto newList = newMods.keys();
    QSet<QString> newSet(newList.begin(), newList.end());
#else
    QSet<QString> currentSet = modsIndex.keys().toSet();
    auto & newMods = m_update->mods;
    QSet<QString> newSet = newMods.keys().toSet();
#endif

    // see if the kept mods changed in some way
    {
        QSet<QString> kept = currentSet;
        kept.intersect(newSet);
        for(auto & keptMod: kept) {
            auto & newMod = newMods[keptMod];
            auto row = modsIndex[keptMod];
            auto & currentMod = mods[row];
            if(newMod.dateTimeChanged() == currentMod.dateTimeChanged()) {
                // no significant change, ignore...
                continue;
            }
            auto & oldMod = mods[row];
            if(oldMod.isResolving()) {
                activeTickets.remove(oldMod.resolutionTicket());
            }
            oldMod = newMod;
            resolveMod(mods[row]);
            emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex()) - 1));
        }
    }

    // remove mods no longer present
    {
        QSet<QString> removed = currentSet;
        QList<int> removedRows;
        removed.subtract(newSet);
        for(auto & removedMod: removed) {
            removedRows.append(modsIndex[removedMod]);
        }
        std::sort(removedRows.begin(), removedRows.end(), std::greater<int>());
        for(auto iter = removedRows.begin(); iter != removedRows.end(); iter++) {
            int removedIndex = *iter;
            beginRemoveRows(QModelIndex(), removedIndex, removedIndex);
            auto removedIter = mods.begin() + removedIndex;
            if(removedIter->isResolving()) {
                activeTickets.remove(removedIter->resolutionTicket());
            }
            mods.erase(removedIter);
            endRemoveRows();
        }
    }

    // add new mods to the end
    {
        QSet<QString> added = newSet;
        added.subtract(currentSet);

        // When you have a Qt build with assertions turned on, proceeding here will abort the application
        if (added.size() > 0) {
            beginInsertRows(QModelIndex(), mods.size(), mods.size() + added.size() - 1);
            for (auto& addedMod : added) {
                mods.append(newMods[addedMod]);
                resolveMod(mods.last());
            }
            endInsertRows();
        }
    }

    // update index
    {
        modsIndex.clear();
        int idx = 0;
        for(auto & mod: mods) {
            modsIndex[mod.internal_id()] = idx;
            idx++;
        }
    }

    m_update.reset();

    emit updateFinished();

    if(scheduled_update) {
        scheduled_update = false;
        update();
    }
}

void ModFolderModel::resolveMod(Mod& m)
{
    if(!m.shouldResolve()) {
        return;
    }

    auto task = new LocalModParseTask(nextResolutionTicket, m.type(), m.fileinfo());
    auto result = task->result();
    result->id = m.internal_id();
    activeTickets.insert(nextResolutionTicket, result);
    m.setResolving(true, nextResolutionTicket);
    nextResolutionTicket++;
    QThreadPool *threadPool = QThreadPool::globalInstance();
    connect(task, &LocalModParseTask::finished, this, &ModFolderModel::finishModParse);
    threadPool->start(task);
}

void ModFolderModel::finishModParse(int token)
{
    auto iter = activeTickets.find(token);
    if(iter == activeTickets.end()) {
        return;
    }
    auto result = *iter;
    activeTickets.remove(token);
    int row = modsIndex[result->id];
    auto & mod = mods[row];
    mod.finishResolvingWithDetails(result->details);
    emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));
}

void ModFolderModel::disableInteraction(bool disabled)
{
    if (interaction_disabled == disabled) {
        return;
    }
    interaction_disabled = disabled;
    if(size()) {
        emit dataChanged(index(0), index(size() - 1));
    }
}

void ModFolderModel::directoryChanged(QString path)
{
    update();
}

bool ModFolderModel::isValid()
{
    return m_dir.exists() && m_dir.isReadable();
}

auto ModFolderModel::selectedMods(QModelIndexList& indexes) -> std::list<Mod>
{
    std::list<Mod> selected_mods;
    for (auto i : indexes) {
        if(i.column() != 0)
            continue;

        selected_mods.push_back(mods[i.row()]);
    }
    return selected_mods;
}

// FIXME: this does not take disabled mod (with extra .disable extension) into account...
bool ModFolderModel::installMod(const QString &filename)
{
    if(interaction_disabled) {
        return false;
    }

    // NOTE: fix for GH-1178: remove trailing slash to avoid issues with using the empty result of QFileInfo::fileName
    auto originalPath = FS::NormalizePath(filename);
    QFileInfo fileinfo(originalPath);

    if (!fileinfo.exists() || !fileinfo.isReadable())
    {
        qWarning() << "Caught attempt to install non-existing file or file-like object:" << originalPath;
        return false;
    }
    qDebug() << "installing: " << fileinfo.absoluteFilePath();

    Mod installedMod(fileinfo);
    if (!installedMod.valid())
    {
        qDebug() << originalPath << "is not a valid mod. Ignoring it.";
        return false;
    }

    auto type = installedMod.type();
    if (type == Mod::MOD_UNKNOWN)
    {
        qDebug() << "Cannot recognize mod type of" << originalPath << ", ignoring it.";
        return false;
    }

    auto newpath = FS::NormalizePath(FS::PathCombine(m_dir.path(), fileinfo.fileName()));
    if(originalPath == newpath)
    {
        qDebug() << "Overwriting the mod (" << originalPath << ") with itself makes no sense...";
        return false;
    }

    if (type == Mod::MOD_SINGLEFILE || type == Mod::MOD_ZIPFILE || type == Mod::MOD_LITEMOD)
    {
        if(QFile::exists(newpath) || QFile::exists(newpath + QString(".disabled")))
        {
            if(!QFile::remove(newpath))
            {
                // FIXME: report error in a user-visible way
                qWarning() << "Copy from" << originalPath << "to" << newpath << "has failed.";
                return false;
            }
            qDebug() << newpath << "has been deleted.";
        }
        if (!QFile::copy(fileinfo.filePath(), newpath))
        {
            qWarning() << "Copy from" << originalPath << "to" << newpath << "has failed.";
            // FIXME: report error in a user-visible way
            return false;
        }
        FS::updateTimestamp(newpath);
        QFileInfo newpathInfo(newpath);
        installedMod.repath(newpathInfo);
        update();
        return true;
    }
    else if (type == Mod::MOD_FOLDER)
    {
        QString from = fileinfo.filePath();
        if(QFile::exists(newpath))
        {
            qDebug() << "Ignoring folder " << from << ", it would merge with " << newpath;
            return false;
        }

        if (!FS::copy(from, newpath)())
        {
            qWarning() << "Copy of folder from" << originalPath << "to" << newpath << "has (potentially partially) failed.";
            return false;
        }
        QFileInfo newpathInfo(newpath);
        installedMod.repath(newpathInfo);
        update();
        return true;
    }
    return false;
}

bool ModFolderModel::setModStatus(const QModelIndexList& indexes, ModStatusAction enable)
{
    if(interaction_disabled) {
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

bool ModFolderModel::deleteMods(const QModelIndexList& indexes)
{
    if(interaction_disabled) {
        return false;
    }

    if(indexes.isEmpty())
        return true;

    for (auto i: indexes)
    {
        if(i.column() != 0) {
            continue;
        }
        Mod &m = mods[i.row()];
        auto index_dir = indexDir();
        m.destroy(index_dir);
    }
    return true;
}

int ModFolderModel::columnCount(const QModelIndex &parent) const
{
    return NUM_COLUMNS;
}

QVariant ModFolderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    int column = index.column();

    if (row < 0 || row >= mods.size())
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        switch (column)
        {
        case NameColumn:
            return mods[row].name();
        case VersionColumn: {
            switch(mods[row].type()) {
                case Mod::MOD_FOLDER:
                    return tr("Folder");
                case Mod::MOD_SINGLEFILE:
                    return tr("File");
                default:
                    break;
            }
            return mods[row].version();
        }
        case DateColumn:
            return mods[row].dateTimeChanged();

        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        return mods[row].internal_id();

    case Qt::CheckStateRole:
        switch (column)
        {
        case ActiveColumn:
            return mods[row].enabled() ? Qt::Checked : Qt::Unchecked;
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

bool ModFolderModel::setModStatus(int row, ModFolderModel::ModStatusAction action)
{
    if(row < 0 || row >= mods.size()) {
        return false;
    }

    auto &mod = mods[row];
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
            desiredStatus = !mod.enabled();
            break;
    }

    if(desiredStatus == mod.enabled()) {
        return true;
    }

    // preserve the row, but change its ID
    auto oldId = mod.internal_id();
    if(!mod.enable(!mod.enabled())) {
        return false;
    }
    auto newId = mod.internal_id();
    if(modsIndex.contains(newId)) {
        // NOTE: this could handle a corner case, where we are overwriting a file, because the same 'mod' exists both enabled and disabled
        // But is it necessary?
    }
    modsIndex.remove(oldId);
    modsIndex[newId] = row;
    emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex()) - 1));
    return true;
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

Qt::ItemFlags ModFolderModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    auto flags = defaultFlags;
    if(interaction_disabled) {
        flags &= ~Qt::ItemIsDropEnabled;
    }
    else
    {
        flags |= Qt::ItemIsDropEnabled;
        if(index.isValid()) {
            flags |= Qt::ItemIsUserCheckable;
        }
    }
    return flags;
}

Qt::DropActions ModFolderModel::supportedDropActions() const
{
    // copy from outside, move from within and other mod lists
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList ModFolderModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}

bool ModFolderModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int, int, const QModelIndex&)
{
    if (action == Qt::IgnoreAction)
    {
        return true;
    }

    // check if the action is supported
    if (!data || !(action & supportedDropActions()))
    {
        return false;
    }

    // files dropped from outside?
    if (data->hasUrls())
    {
        auto urls = data->urls();
        for (auto url : urls)
        {
            // only local files may be dropped...
            if (!url.isLocalFile())
            {
                continue;
            }
            // TODO: implement not only copy, but also move
            // FIXME: handle errors here
            installMod(url.toLocalFile());
        }
        return true;
    }
    return false;
}
