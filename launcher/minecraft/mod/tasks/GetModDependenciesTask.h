// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
#include <QList>
#include <QVariant>
#include <memory>

#include "api/structures/Project.h"
#include "minecraft/mod/MetadataHandler.h"
#include "minecraft/mod/ModFolderModel.h"
#include "tasks/SequentialTask.h"
#include "tasks/Task.h"
#include "ui/pages/modplatform/ModModel.h"

class GetModDependenciesTask : public SequentialTask {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<GetModDependenciesTask>;

    struct PackDependency {
        Platform::Dependency dependency;
        Platform::Project::Ptr pack;
        Platform::Version version;
        PackDependency() = default;
        PackDependency(const Platform::Project::Ptr p, const Platform::Version& v)
        {
            pack = p;
            version = v;
        }
    };

    struct PackDependencyExtraInfo {
        bool maybe_installed;
        QStringList required_by;
    };

    explicit GetModDependenciesTask(BaseInstance* instance, ModFolderModel* folder, QList<std::shared_ptr<PackDependency>> selected);

    auto getDependecies() const -> QList<std::shared_ptr<PackDependency>> { return m_pack_dependencies; }
    QHash<QString, PackDependencyExtraInfo> getExtraInfo();

   private:
   protected slots:
    Task::Ptr prepareDependencyTask(const Platform::Dependency&, Platform::Provider, int);
    QList<Platform::Dependency> getDependenciesForVersion(const Platform::Version&, Platform::Provider providerName);
    void prepare();
    Task::Ptr getProjectInfoTask(std::shared_ptr<PackDependency> pDep);
    Platform::Dependency getOverride(const Platform::Dependency&, Platform::Provider providerName);
    void removePack(const QVariant& addonId);

    bool isLocalyInstalled(std::shared_ptr<PackDependency> pDep);
    bool maybeInstalled(std::shared_ptr<PackDependency> pDep);

   private:
    QList<std::shared_ptr<PackDependency>> m_pack_dependencies;
    QList<std::shared_ptr<Metadata::ModStruct>> m_mods;
    QList<std::shared_ptr<PackDependency>> m_selected;
    QStringList m_mods_file_names;

    Version m_version;
    Platform::ModLoaders m_loaderType;
};
