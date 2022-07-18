// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include <QString>
#include <QList>
#include <list>

#include "Version.h"
#include "net/NetJob.h"

namespace ModPlatform {
class ListModel;
struct IndexedPack;
}

class ModAPI {
   protected:
    using CallerType = ModPlatform::ListModel;

   public:
    virtual ~ModAPI() = default;

    enum ModLoaderType {
        Unspecified = 0,
        Forge = 1 << 0,
        Cauldron = 1 << 1,
        LiteLoader = 1 << 2,
        Fabric = 1 << 3,
        Quilt = 1 << 4
    };
    Q_DECLARE_FLAGS(ModLoaderTypes, ModLoaderType)

    struct SearchArgs {
        int offset;
        QString search;
        QString sorting;
        ModLoaderTypes loaders;
        std::list<Version> versions;
    };

    virtual void searchMods(CallerType* caller, SearchArgs&& args) const = 0;
    virtual void getModInfo(ModPlatform::IndexedPack& pack, std::function<void(QJsonDocument&, ModPlatform::IndexedPack&)> callback) = 0;

    virtual auto getProject(QString addonId, QByteArray* response) const -> NetJob* = 0;
    virtual auto getProjects(QStringList addonIds, QByteArray* response) const -> NetJob* = 0;


    struct VersionSearchArgs {
        QString addonId;
        std::list<Version> mcVersions;
        ModLoaderTypes loaders;
    };

    virtual void getVersions(CallerType* caller, VersionSearchArgs&& args) const = 0;

    static auto getModLoaderString(ModLoaderType type) -> const QString {
        switch (type) {
            case Unspecified:
                break;
            case Forge:
                return "forge";
            case Cauldron:
                return "cauldron";
            case LiteLoader:
                return "liteloader";
            case Fabric:
                return "fabric";
            case Quilt:
                return "quilt";
        }
        return "";
    }

   protected:
    inline auto getGameVersionsString(std::list<Version> mcVersions) const -> QString
    {
        QString s;
        for(auto& ver : mcVersions){
            s += QString("\"%1\",").arg(ver.toString());
        }
        s.remove(s.length() - 1, 1); //remove last comma
        return s;
    }
};
