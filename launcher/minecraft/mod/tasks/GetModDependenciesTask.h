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
#include <QEventLoop>
#include <QList>
#include <QVariant>
#include <functional>
#include <memory>

#include "minecraft/mod/MetadataHandler.h"
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "tasks/SequentialTask.h"
#include "tasks/Task.h"
#include "ui/pages/modplatform/ModModel.h"

class GetModDependenciesTask : public SequentialTask {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<GetModDependenciesTask>;

    struct PackDependecny {
        ModPlatform::Dependency dependency;
        ModPlatform::IndexedPack pack;
        ModPlatform::IndexedVersion version;
        PackDependecny(){};
        PackDependecny(const ModPlatform::IndexedPack& p, const ModPlatform::IndexedVersion& v)
        {
            pack = p;
            version = v;
        }
    };

    struct Provider {
        ModPlatform::ResourceProvider name;
        std::shared_ptr<ResourceDownload::ModModel> mod;
        std::shared_ptr<ResourceAPI> api;
    };

    explicit GetModDependenciesTask(QObject* parent,
                                    BaseInstance* instance,
                                    ModFolderModel* folder,
                                    QList<std::shared_ptr<PackDependecny>> selected);

    auto getDependecies() const -> QList<std::shared_ptr<PackDependecny>> { return m_pack_dependencies; }

   protected slots:
    Task::Ptr prepareDependencyTask(const ModPlatform::Dependency&, const ModPlatform::ResourceProvider, int);
    QList<ModPlatform::Dependency> getDependenciesForVersion(const ModPlatform::IndexedVersion&,
                                                             const ModPlatform::ResourceProvider providerName);
    void prepare();

   private:
    QList<std::shared_ptr<PackDependecny>> m_pack_dependencies;
    QList<std::shared_ptr<Metadata::ModStruct>> m_mods;
    QList<std::shared_ptr<PackDependecny>> m_selected;
    Provider m_flame_provider;
    Provider m_modrinth_provider;

    Version m_version;
    ResourceAPI::ModLoaderTypes m_loaderType;
};
