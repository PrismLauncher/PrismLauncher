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
 */

#include "GetModDependenciesTask.h"

#include "QObjectPtr.h"
#include "minecraft/mod/MetadataHandler.h"
#include "minecraft/mod/tasks/LocalModGetAllTask.h"
#include "modplatform/ModIndex.h"
#include "tasks/ConcurrentTask.h"
#include "tasks/SequentialTask.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

GetModDependenciesTask::GetModDependenciesTask(QDir index_dir, QList<ModPlatform::IndexedVersion> selected, NewDependecyVersionAPITask& api)
    : m_selected(selected), m_getDependenciesVersionAPI(api)
{
    m_getAllMods = makeShared<LocalModGetAllTask>(index_dir);
    m_getNetworkDep = makeShared<SequentialTask>(this, "GetDepInfo");
    QObject::connect(m_getAllMods.get(), &LocalModGetAllTask::getAllMod, [this](QList<Metadata::ModStruct> mods) {
        m_mods = mods;
        prepareDependecies();
    });

#ifdef Q_OS_WIN32
    SetFileAttributesW(index_dir.path().toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
#endif
}

void GetModDependenciesTask::executeTask()
{
    setStatus(tr("Geting all mods"));
    m_getAllMods->start();
}

auto GetModDependenciesTask::abort() -> bool
{
    emitAborted();
    return true;
}

void GetModDependenciesTask::prepareDependecies()
{
    auto c_dependencies = getDependenciesForVersions(m_selected);
    if (c_dependencies.length() == 0) {
        emitSucceeded();
        return;
    }
    for (auto dep : c_dependencies) {
        auto task = m_getDependenciesVersionAPI(
            dep, 20, [this](QList<ModPlatform::IndexedVersion> new_versions, int level) { addDependecies(new_versions, level - 1); });
        m_getNetworkDep->addTask(task);
    }
    m_getNetworkDep->start();
}

void GetModDependenciesTask::addDependecies(QList<ModPlatform::IndexedVersion> new_versions, int level)
{
    // some mutex?
    m_dependencies.append(new_versions);
    auto c_dependencies = getDependenciesForVersions(m_selected);
    if (c_dependencies.length() == 0) {
        return;
    }
    if (level == 0) {
        qWarning() << "Dependency cycle exeeded";
    }
    for (auto dep : c_dependencies) {
        auto task = m_getDependenciesVersionAPI(
            dep, 20, [this](QList<ModPlatform::IndexedVersion> new_versions, int level) { addDependecies(new_versions, level - 1); });
        m_getNetworkDep->addTask(task);
    }
};

QList<ModPlatform::Dependency> GetModDependenciesTask::getDependenciesForVersions(QList<ModPlatform::IndexedVersion> selected)
{
    auto c_dependencies = QList<ModPlatform::Dependency>();
    for (auto version : selected) {
        for (auto ver_dep : version.dependencies) {
            if (ver_dep.type == ModPlatform::DependencyType::REQUIRED) {
                if (auto dep = std::find_if(c_dependencies.begin(), c_dependencies.end(),
                                            [ver_dep](auto i) { return i.addonId == ver_dep.addonId; });
                    dep == c_dependencies.end()) {  // check the current dependency list
                    c_dependencies.append(ver_dep);
                } else if (auto dep =
                               std::find_if(selected.begin(), selected.end(), [ver_dep](auto i) { return i.addonId == ver_dep.addonId; });
                           dep == selected.end()) {  // check the selected versions
                    c_dependencies.append(ver_dep);
                } else if (auto dep =
                               std::find_if(m_mods.begin(), m_mods.end(), [ver_dep](auto i) { return i.mod_id() == ver_dep.addonId; });
                           dep == m_mods.end()) {  // check the existing mods
                    c_dependencies.append(ver_dep);
                }
            }
        }
    }
    return c_dependencies;
};
