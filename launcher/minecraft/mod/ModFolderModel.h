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

#pragma once

#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QDir>
#include <QAbstractListModel>

#include "Mod.h"

#include "minecraft/mod/tasks/ModFolderLoadTask.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"

class LegacyInstance;
class BaseInstance;
class QFileSystemWatcher;

/**
 * A legacy mod list.
 * Backed by a folder.
 */
class ModFolderModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Columns
    {
        ActiveColumn = 0,
        NameColumn,
        VersionColumn,
        DateColumn,
        NUM_COLUMNS
    };
    enum ModStatusAction {
        Disable,
        Enable,
        Toggle
    };
    ModFolderModel(const QString &dir, bool is_indexed = false);

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::DropActions supportedDropActions() const override;

    /// flags, mostly to support drag&drop
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    QStringList mimeTypes() const override;
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) override;

    virtual int rowCount(const QModelIndex &) const override
    {
        return size();
    }

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual int columnCount(const QModelIndex &parent) const override;

    size_t size() const
    {
        return mods.size();
    }
    ;
    bool empty() const
    {
        return size() == 0;
    }
    Mod &operator[](size_t index)
    {
        return mods[index];
    }
    const Mod &at(size_t index) const
    {
        return mods.at(index);
    }

    /// Reloads the mod list and returns true if the list changed.
    bool update();

    /**
     * Adds the given mod to the list at the given index - if the list supports custom ordering
     */
    bool installMod(const QString& filename);

    /// Deletes all the selected mods
    bool deleteMods(const QModelIndexList &indexes);

    /// Enable or disable listed mods
    bool setModStatus(const QModelIndexList &indexes, ModStatusAction action);

    void startWatching();
    void stopWatching();

    bool isValid();

    QDir& dir()
    {
        return m_dir;
    }

    QDir indexDir()
    {
        return { QString("%1/.index").arg(dir().absolutePath()) };
    }

    const QList<Mod> & allMods()
    {
        return mods;
    }

    auto selectedMods(QModelIndexList& indexes) -> std::list<Mod>;

public slots:
    void disableInteraction(bool disabled);

private
slots:
    void directoryChanged(QString path);
    void finishUpdate();
    void finishModParse(int token);

signals:
    void updateFinished();

private:
    void resolveMod(Mod& m);
    bool setModStatus(int index, ModStatusAction action);

protected:
    QFileSystemWatcher *m_watcher;
    bool is_watching = false;
    ModFolderLoadTask::ResultPtr m_update;
    bool scheduled_update = false;
    bool interaction_disabled = false;
    QDir m_dir;
    bool m_is_indexed;
    QMap<QString, int> modsIndex;
    QMap<int, LocalModParseTask::ResultPtr> activeTickets;
    int nextResolutionTicket = 0;
    QList<Mod> mods;
};
