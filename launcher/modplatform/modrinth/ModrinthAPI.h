#pragma once

#include "modplatform/helpers/NetworkModAPI.h"

#include <QDebug>

class ModrinthAPI : public NetworkModAPI {
   public:
    inline auto getAuthorURL(const QString& name) const -> QString { return "https://modrinth.com/user/" + name; };

   private:
    inline auto getModSearchURL(SearchArgs& args) const -> QString override
    {
        if (!validateModLoader(args.mod_loader)) {
            qWarning() << "Modrinth only have Forge and Fabric-compatible mods!";
            return "";
        }

        return QString(
                   "https://api.modrinth.com/v2/search?"
                   "offset=%1&"
                   "limit=25&"
                   "query=%2&"
                   "index=%3&"
                   "facets=[[\"categories:%4\"],%5[\"project_type:mod\"]]")
            .arg(args.offset)
            .arg(args.search)
            .arg(args.sorting)
            .arg(getModLoaderString(args.mod_loader))
            .arg(getGameVersionsArray(args.versions));
    };

    inline auto getVersionsURL(VersionSearchArgs& args) const -> QString override
    {
        return QString("https://api.modrinth.com/v2/project/%1/version?"
                "game_versions=[%2]"
                "loaders=[%3]")
            .arg(args.addonId)
            .arg(getGameVersionsString(args.mcVersions))
            .arg(getModLoaderString(args.loader));
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

    static auto getModLoaderString(ModLoaderType type) -> const QString
    {
        if (type == Unspecified)
            return "fabric, forge, quilt";
        // TODO: remove this once Quilt drops official Fabric support
        if (type == Quilt)  // NOTE: Most if not all Fabric mods should work *currently*
            return "fabric, quilt";
        return ModAPI::getModLoaderString(type);
    }

    inline auto validateModLoader(ModLoaderType modLoader) const -> bool
    {
        return modLoader == Unspecified || modLoader == Forge || modLoader == Fabric || modLoader == Quilt;
    }

};
