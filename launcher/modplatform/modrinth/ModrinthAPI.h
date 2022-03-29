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
                   "facets=[[\"categories:%4\"],[\"versions:%5\"],[\"project_type:mod\"]]")
            .arg(args.offset)
            .arg(args.search)
            .arg(args.sorting)
            .arg(getModLoaderString(args.mod_loader))
            .arg(args.version);
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

    inline auto getGameVersionsString(QList<QString> mcVersions) const -> QString
    {
        QString s;
        for(int i = 0; i < mcVersions.count(); i++){
            s += mcVersions.at(i);
            if(i < mcVersions.count() - 1)
                s += ",";
        }
        return s;
    }

    inline auto getModLoaderString(ModLoaderType modLoader) const -> QString
    {
        switch (modLoader) {
            case Any:
                return "fabric, forge";
            case Forge:
                return "forge";
            case Fabric:
                return "fabric";
            default:
                return "";
        }
    }

    inline auto validateModLoader(ModLoaderType modLoader) const -> bool
    {
        return modLoader == Any || modLoader == Forge || modLoader == Fabric;
    }

};
