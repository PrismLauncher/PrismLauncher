/* Copyright 2013-2018 MultiMC Contributors
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

#include "minecraft/Mod.h"

#include "multimc_logic_export.h"

class LegacyInstance;
class BaseInstance;
class QFileSystemWatcher;

/**
 * A legacy mod list.
 * Backed by a folder.
 */
class MULTIMC_LOGIC_EXPORT SimpleModList : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Columns
    {
        ActiveColumn = 0,
        NameColumn,
        DateColumn,
        VersionColumn,
        NUM_COLUMNS
    };
    SimpleModList(const QString &dir);

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

    /// Reloads the mod list and returns true if the list changed.
    virtual bool update();

    /**
     * Adds the given mod to the list at the given index - if the list supports custom ordering
     */
    bool installMod(const QString& filename);

    /// Deletes all the selected mods
    virtual bool deleteMods(const QModelIndexList &indexes);

    /// Enable or disable listed mods
    virtual bool enableMods(const QModelIndexList &indexes, bool enable = true);

    void startWatching();
    void stopWatching();

    virtual bool isValid();

    QDir dir()
    {
        return m_dir;
    }

    const QList<Mod> & allMods()
    {
        return mods;
    }

private
slots:
    void directoryChanged(QString path);

signals:
    void changed();

protected:
    QFileSystemWatcher *m_watcher;
    bool is_watching = false;
    QDir m_dir;
    QList<Mod> mods;
};
