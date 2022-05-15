// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include "BuildConfig.h"
#include "modplatform/ModAPI.h"
#include "modplatform/helpers/NetworkModAPI.h"

#include <QDebug>

class ModrinthAPI : public NetworkModAPI {
   public:
    inline auto getAuthorURL(const QString& name) const -> QString { return "https://modrinth.com/user/" + name; };

    static auto getModLoaderStrings(ModLoaderType type) -> const QStringList
    {
        QStringList l;
        switch (type)
        {
            case Unspecified:
                for (auto loader : {Forge, Fabric, Quilt})
                {
                    l << ModAPI::getModLoaderString(loader);
                }
                break;

            case Quilt:
                l << ModAPI::getModLoaderString(Fabric);
            default:
                l << ModAPI::getModLoaderString(type);
        }
        return l;
    }

    static auto getModLoaderFilters(ModLoaderType type) -> const QString
    {
        QStringList l;
        for (auto loader : getModLoaderStrings(type))
        {
            l << QString("\"categories:%1\"").arg(loader);
        }
        return l.join(',');
    }

   private:
    inline auto getModSearchURL(SearchArgs& args) const -> QString override
    {
        if (!validateModLoader(args.mod_loader)) {
            qWarning() << "Modrinth only have Forge and Fabric-compatible mods!";
            return "";
        }

        return QString(BuildConfig.MODRINTH_PROD_URL +
                       "/search?"
                       "offset=%1&"
                       "limit=25&"
                       "query=%2&"
                       "index=%3&"
                       "facets=[[%4],%5[\"project_type:mod\"]]")
            .arg(args.offset)
            .arg(args.search)
            .arg(args.sorting)
            .arg(getModLoaderFilters(args.mod_loader))
            .arg(getGameVersionsArray(args.versions));
    };

    inline auto getVersionsURL(VersionSearchArgs& args) const -> QString override
    {
        return QString(BuildConfig.MODRINTH_PROD_URL +
                       "/project/%1/version?"
                       "game_versions=[%2]"
                       "loaders=[\"%3\"]")
            .arg(args.addonId)
            .arg(getGameVersionsString(args.mcVersions))
            .arg(getModLoaderStrings(args.loader).join("\",\""));
    };

    auto getGameVersionsArray(std::list<Version> mcVersions) const -> QString
    {
        QString s;
        for(auto& ver : mcVersions){
            s += QString("\"versions:%1\",").arg(ver.toString());
        }
        s.remove(s.length() - 1, 1); //remove last comma
        return s.isEmpty() ? QString() : QString("[%1],").arg(s);
    }

    inline auto validateModLoader(ModLoaderType modLoader) const -> bool
    {
        return modLoader == Unspecified || modLoader == Forge || modLoader == Fabric || modLoader == Quilt;
    }

};
