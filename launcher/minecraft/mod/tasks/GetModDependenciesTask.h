// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
 */

#pragma once

#include <QDir>
#include <functional>

#include "minecraft/mod/MetadataHandler.h"
#include "minecraft/mod/tasks/LocalModGetAllTask.h"
#include "modplatform/ModIndex.h"
#include "tasks/SequentialTask.h"
#include "tasks/Task.h"

class GetModDependenciesTask : public Task {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<GetModDependenciesTask>;
    using LocalModGetAllTaskPtr = shared_qobject_ptr<LocalModGetAllTask>;

    using NewDependecyVersionAPITask =
        std::function<Task::Ptr(ModPlatform::Dependency, int, std::function<void(QList<ModPlatform::IndexedVersion>, int)>)>;

    explicit GetModDependenciesTask(QDir index_dir, QList<ModPlatform::IndexedVersion> selected, NewDependecyVersionAPITask& api);

    auto canAbort() const -> bool override { return true; }
    auto abort() -> bool override;

   protected slots:
    //! Entry point for tasks.
    void executeTask() override;

    void prepareDependecies();
    void addDependecies(QList<ModPlatform::IndexedVersion>, int);
    QList<ModPlatform::Dependency> getDependenciesForVersions(QList<ModPlatform::IndexedVersion>);

   signals:
    void getAllMod(QList<Metadata::ModStruct>);

   private:
    QList<ModPlatform::IndexedVersion> m_selected;
    QList<ModPlatform::IndexedVersion> m_dependencies;
    QList<Metadata::ModStruct> m_mods;

    LocalModGetAllTaskPtr m_getAllMods = nullptr;
    NewDependecyVersionAPITask m_getDependenciesVersionAPI;
    SequentialTask::Ptr m_getNetworkDep;
};
