/* Copyright 2015-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QList>
#include <QString>
#include <QDir>
#include <QAbstractListModel>
#include <QMimeData>
#include "minecraft/World.h"
#include "BaseInstance.h"

class QFileSystemWatcher;

class WorldList : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Columns
    {
        NameColumn,
        GameModeColumn,
        LastPlayedColumn,
        SizeColumn,
        InfoColumn
    };

    enum Roles
    {
        ObjectRole = Qt::UserRole + 1,
        FolderRole,
        SeedRole,
        NameRole,
        GameModeRole,
        LastPlayedRole,
        SizeRole,
        IconFileRole
    };

    WorldList(const QString &dir, BaseInstance* instance);

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        return parent.isValid() ? 0 : static_cast<int>(size());
    };
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual int columnCount(const QModelIndex &parent) const;

    size_t size() const
    {
        return worlds.size();
    };
    bool empty() const
    {
        return size() == 0;
    }
    World &operator[](size_t index)
    {
        return worlds[index];
    }

    /// Reloads the mod list and returns true if the list changed.
    virtual bool update();

    /// Install a world from location
    void installWorld(QFileInfo filename);

    /// Deletes the mod at the given index.
    virtual bool deleteWorld(int index);

    /// Removes the world icon, if any
    virtual bool resetIcon(int index);

    /// Deletes all the selected mods
    virtual bool deleteWorlds(int first, int last);

    /// flags, mostly to support drag&drop
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    /// get data for drag action
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    /// get the supported mime types
    virtual QStringList mimeTypes() const;
    /// process data from drop action
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    /// what drag actions do we support?
    virtual Qt::DropActions supportedDragActions() const;

    /// what drop actions do we support?
    virtual Qt::DropActions supportedDropActions() const;

    void startWatching();
    void stopWatching();

    virtual bool isValid();

    QDir dir() const
    {
        return m_dir;
    }

    QString instDirPath() const;

    const QList<World> &allWorlds() const
    {
        return worlds;
    }

private slots:
    void directoryChanged(QString path);

signals:
    void changed();

protected:
    BaseInstance* m_instance;
    QFileSystemWatcher *m_watcher;
    bool is_watching;
    QDir m_dir;
    QList<World> worlds;
};
