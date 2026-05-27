// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "MultiWorldList.h"

#include <FileSystem.h>
#include <QDebug>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QMimeData>
#include <QString>
#include <QThreadPool>
#include <QUrl>
#include <QUuid>
#include <Qt>

MultiWorldList::MultiWorldList(const QList<QString>& dirs, QList<BaseInstance*> instances) : QAbstractListModel(), m_instances(instances)
{
    for (int i = 0; i < dirs.length(); i++) { // better way to do this? iy
        m_dirs[i] = dirs[i];
    }

    for (QDir dir : m_dirs) {
        FS::ensureFolderPathExists(dir.absolutePath());
        dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
        dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
    }

    m_watcher = new QFileSystemWatcher(this);
    m_isWatching = false;
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &MultiWorldList::directoryChanged);
}

void MultiWorldList::startWatching() //figure out what watchers do / if necessary and do all paths iy
{
    if (m_isWatching) {
        return;
    }
    update();
    m_isWatching = m_watcher->addPath(m_dirs[0].absolutePath());
    if (m_isWatching) {
        qDebug() << "Started watching" << m_dirs[0].absolutePath();
    } else {
        qDebug() << "Failed to start watching" << m_dirs[0].absolutePath();
    }
}

void MultiWorldList::stopWatching() //same as above function iy
{
    if (!m_isWatching) {
        return;
    }
    m_isWatching = !m_watcher->removePath(m_dirs[0].absolutePath());
    if (!m_isWatching) {
        qDebug() << "Stopped watching" << m_dirs[0].absolutePath();
    } else {
        qDebug() << "Failed to stop watching" << m_dirs[0].absolutePath();
    }
}

bool MultiWorldList::update()
{
    if (!isValid())
        return false;

    QList<World> newWorlds;

    for (QDir dir : m_dirs) {
        dir.refresh();
        auto folderContents = dir.entryInfoList();
        // if there are any untracked files...
        for (QFileInfo entry : folderContents) {
            if (!entry.isDir())
                continue;

            World w(entry);
            if (w.isValid()) {
                newWorlds.append(w);
            }
        }
    }
    
    beginResetModel();
    m_worlds.swap(newWorlds);
    endResetModel();
    loadWorldsAsync();
    return true;
}

void MultiWorldList::directoryChanged(QString)
{
    update();
}

bool MultiWorldList::isValid() //account for all directories
{
    return m_dirs[0].exists() && m_dirs[0].isReadable();
}

QList<QString> MultiWorldList::instDirPaths() const
{
    QList<QString> dirList;

    for (BaseInstance* instance : m_instances) {
        dirList.append(QFileInfo(instance->instanceRoot()).absoluteFilePath());
    }

    return dirList;
}

bool MultiWorldList::deleteWorld(int index)
{
    if (index >= m_worlds.size() || index < 0)
        return false;
    World& m = m_worlds[index];
    if (m.destroy()) {
        beginRemoveRows(QModelIndex(), index, index);
        m_worlds.removeAt(index);
        endRemoveRows();
        emit changed();
        return true;
    }
    return false;
}

bool MultiWorldList::deleteWorlds(int first, int last)
{
    for (int i = first; i <= last; i++) {
        World& m = m_worlds[i];
        m.destroy();
    }
    beginRemoveRows(QModelIndex(), first, last);
    m_worlds.erase(m_worlds.begin() + first, m_worlds.begin() + last + 1);
    endRemoveRows();
    emit changed();
    return true;
}

bool MultiWorldList::resetIcon(int row)
{
    if (row >= m_worlds.size() || row < 0)
        return false;
    World& m = m_worlds[row];
    if (m.resetIcon()) {
        QModelIndex modelIndex = index(row, NameColumn);
        emit dataChanged(modelIndex, modelIndex, { MultiWorldList::IconFileRole });
        return true;
    }
    return false;
}

int MultiWorldList::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 5;
}

QVariant MultiWorldList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    int column = index.column();

    if (row < 0 || row >= m_worlds.size())
        return QVariant();

    QLocale locale;

    auto& world = m_worlds[row];
    switch (role) {
        case Qt::DisplayRole:
            switch (column) {
                case NameColumn:
                    return world.name();

                case GameModeColumn:
                    return world.gameType().toTranslatedString();

                case LastPlayedColumn:
                    return world.lastPlayed();

                case SizeColumn:
                    return locale.formattedDataSize(world.bytes());

                case InfoColumn:
                    if (world.isSymLinkUnder(instDirPaths()[0])) { //FIX THIS--NO INDEX 0 iy
                        return tr("This world is symbolically linked from elsewhere.");
                    }
                    if (world.isMoreThanOneHardLink()) {
                        return tr("\nThis world is hard linked elsewhere.");
                    }
                    return "";
                default:
                    return QVariant();
            }

        case Qt::UserRole:
            if (column == SizeColumn)
                return QVariant::fromValue<qlonglong>(world.bytes());
            return data(index, Qt::DisplayRole);

        case Qt::ToolTipRole: {
            if (column == InfoColumn) {
                if (world.isSymLinkUnder(instDirPaths()[0])) { //SAME HERE iy
                    return tr("Warning: This world is symbolically linked from elsewhere. Editing it will also change the original."
                              "\nCanonical Path: %1")
                        .arg(world.canonicalFilePath());
                }
                if (world.isMoreThanOneHardLink()) {
                    return tr("Warning: This world is hard linked elsewhere. Editing it will also change the original.");
                }
            }
            return world.folderName();
        }
        case ObjectRole: {
            return QVariant::fromValue<void*>((void*)&world);
        }
        case FolderRole: {
            return QDir::toNativeSeparators(dirs()[0].absoluteFilePath(world.folderName())); //SAME HERE iy
        }
        case SeedRole: {
            return QVariant::fromValue<qlonglong>(world.seed());
        }
        case NameRole: {
            return world.name();
        }
        case LastPlayedRole: {
            return world.lastPlayed();
        }
        case SizeRole: {
            return QVariant::fromValue<qlonglong>(world.bytes());
        }
        case IconFileRole: {
            return world.iconFile();
        }
        default:
            return QVariant();
    }
}

QVariant MultiWorldList::headerData(int section, [[maybe_unused]] Qt::Orientation orientation, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case NameColumn:
                    return tr("Name");
                case GameModeColumn:
                    return tr("Game Mode");
                case LastPlayedColumn:
                    return tr("Last Played");
                case SizeColumn:
                    //: World size on disk
                    return tr("Size");
                case InfoColumn:
                    //: special warnings?
                    return tr("Info");
                default:
                    return QVariant();
            }

        case Qt::ToolTipRole:
            switch (section) {
                case NameColumn:
                    return tr("The name of the world.");
                case GameModeColumn:
                    return tr("Game mode of the world.");
                case LastPlayedColumn:
                    return tr("Date and time the world was last played.");
                case SizeColumn:
                    return tr("Size of the world on disk.");
                case InfoColumn:
                    return tr("Information and warnings about the world.");
                default:
                    return QVariant();
            }
        default:
            return QVariant();
    }
}

QStringList MultiWorldList::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}

QMimeData* MultiWorldList::mimeData(const QModelIndexList& indexes) const
{
    QList<QUrl> urls;

    for (auto idx : indexes) {
        if (idx.column() != 0)
            continue;

        int row = idx.row();
        if (row < 0 || row >= this->m_worlds.size())
            continue;

        const World& world = m_worlds[row];

        if (!world.isValid() || !world.isOnFS())
            continue;

        QString worldPath = world.container().absoluteFilePath();
        qDebug() << worldPath;
        urls.append(QUrl::fromLocalFile(worldPath));
    }

    auto result = new QMimeData();
    result->setUrls(urls);
    return result;
}

Qt::ItemFlags MultiWorldList::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    if (index.isValid())
        return Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}

Qt::DropActions MultiWorldList::supportedDragActions() const
{
    // move to other mod lists or VOID
    return Qt::MoveAction;
}

Qt::DropActions MultiWorldList::supportedDropActions() const
{
    // copy from outside, move from within and other mod lists
    return Qt::CopyAction | Qt::MoveAction;
}

void MultiWorldList::installWorld(QFileInfo filename)
{
    qDebug() << "installing:" << filename.absoluteFilePath();
    World w(filename);
    if (!w.isValid()) {
        return;
    }
    w.install(m_dirs[0].absolutePath()); //more directory stuff iy
}

bool MultiWorldList::dropMimeData(const QMimeData* data,
                             Qt::DropAction action,
                             [[maybe_unused]] int row,
                             [[maybe_unused]] int column,
                             [[maybe_unused]] const QModelIndex& parent)
{
    if (action == Qt::IgnoreAction)
        return true;
    // check if the action is supported
    if (!data || !(action & supportedDropActions()))
        return false;
    // files dropped from outside?
    if (data->hasUrls()) {
        bool was_watching = m_isWatching;
        if (was_watching)
            stopWatching();
        auto urls = data->urls();
        for (auto url : urls) {
            // only local files may be dropped...
            if (!url.isLocalFile())
                continue;
            QString filename = url.toLocalFile();

            QFileInfo worldInfo(filename);

            if (!m_dirs[0].entryInfoList().contains(worldInfo)) { //more stuff to fix iy
                installWorld(worldInfo);
            }
        }
        if (was_watching)
            startWatching();
        return true;
    }
    return false;
}

int64_t MultiWorldList::calculateWorldSize(const QFileInfo& file)
{
    if (file.isFile() && file.suffix() == "zip") {
        return file.size();
    } else if (file.isDir()) {
        QDirIterator it(file.absoluteFilePath(), QDir::Files, QDirIterator::Subdirectories);
        int64_t total = 0;
        while (it.hasNext()) {
            it.next();
            total += it.fileInfo().size();
        }
        return total;
    }
    return -1;
}

void MultiWorldList::loadWorldsAsync()
{
    for (int i = 0; i < m_worlds.size(); ++i) {
        auto file = m_worlds.at(i).container();
        int row = i;
        QThreadPool::globalInstance()->start([this, file, row]() mutable {
            auto size = calculateWorldSize(file);

            QMetaObject::invokeMethod(
                this,
                [this, size, row, file]() {
                    if (row < m_worlds.size() && m_worlds[row].container() == file) {
                        m_worlds[row].setSize(size);

                        // Notify views
                        QModelIndex modelIndex = index(row, SizeColumn);
                        emit dataChanged(modelIndex, modelIndex, { SizeRole });
                    }
                },
                Qt::QueuedConnection);
        });
    }
}
