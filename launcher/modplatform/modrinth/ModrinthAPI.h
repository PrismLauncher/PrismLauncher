#pragma once

#include "modplatform/helpers/NetworkModAPI.h"

#include <QDebug>

class ModrinthAPI : public NetworkModAPI {
   public:
    inline QString getAuthorURL(const QString& name) const { return "https://modrinth.com/user/" + name; };

   private:
    inline QString getModSearchURL(SearchArgs& args) const override
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

    inline QString getVersionsURL(const QString& addonId) const override
    {
        return QString("https://api.modrinth.com/v2/project/%1/version").arg(addonId);
    };

    inline QString getModLoaderString(ModLoaderType modLoader) const
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

    inline bool validateModLoader(ModLoaderType modLoader) const
    {
        return modLoader == Any || modLoader == Forge || modLoader == Fabric;
    }

};
