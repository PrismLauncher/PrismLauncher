// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include <QAbstractListModel>
#include <QDir>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>

#include "Mod.h"
#include "ResourceFolderModel.h"

#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "minecraft/mod/tasks/ModFolderLoadTask.h"
#include "modplatform/ModIndex.h"

class LegacyInstance;
class BaseInstance;
class QFileSystemWatcher;

/**
 * A legacy mod list.
 * Backed by a folder.
 */
class ModFolderModel : public ResourceFolderModel {
    Q_OBJECT
   public:
    enum Columns { ActiveColumn = 0, ImageColumn, NameColumn, VersionColumn, DateColumn, ProviderColumn, NUM_COLUMNS };
    enum ModStatusAction { Disable, Enable, Toggle };
    ModFolderModel(const QString& dir, BaseInstance* instance, bool is_indexed = false, bool create_dir = true);

    virtual QString id() const override { return "mods"; }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex& parent) const override;

    [[nodiscard]] Task* createUpdateTask() override;
    [[nodiscard]] Task* createParseTask(Resource&) override;

    bool installMod(QString file_path) { return ResourceFolderModel::installResource(file_path); }
    bool installMod(QString file_path, ModPlatform::IndexedVersion& vers);
    bool uninstallMod(const QString& filename, bool preserve_metadata = false);

    /// Deletes all the selected mods
    bool deleteMods(const QModelIndexList& indexes);

    bool isValid();

    bool startWatching() override;
    bool stopWatching() override;

    QDir indexDir() { return { QString("%1/.index").arg(dir().absolutePath()) }; }

    auto selectedMods(QModelIndexList& indexes) -> QList<Mod*>;
    auto allMods() -> QList<Mod*>;

    RESOURCE_HELPERS(Mod)

   private slots:
    void onUpdateSucceeded() override;
    void onParseSucceeded(int ticket, QString resource_id) override;

   protected:
    bool m_is_indexed;
    bool m_first_folder_load = true;
};
