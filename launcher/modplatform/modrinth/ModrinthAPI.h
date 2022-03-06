#pragma once

#include "modplatform/ModAPI.h"

#include <QDebug>

class ModrinthAPI : public ModAPI {
   public:
    inline QString getModSearchURL(int offset, QString query, QString sort, ModLoaderType modLoader, QString version) const override 
    { 
        if(!validateModLoader(modLoader)){
            qWarning() << "Modrinth only have Forge and Fabric-compatible mods!";
            return "";
        }

        return QString("https://api.modrinth.com/v2/search?"
                "offset=%1&"   "limit=25&"   "query=%2&"   "index=%3&"
                "facets=[[\"categories:%4\"],[\"versions:%5\"],[\"project_type:mod\"]]")
                  .arg(offset)
                  .arg(query)
                  .arg(sort)
                  .arg(getModLoaderString(modLoader))
                  .arg(version);
    };

    inline QString getVersionsURL(const QString& addonId) const override
    {
        return QString("https://api.modrinth.com/v2/project/%1/version").arg(addonId);
    };

    inline QString getAuthorURL(const QString& name) const override { return "https://modrinth.com/user/" + name; };

   private:
    inline bool validateModLoader(ModLoaderType modLoader) const{
        return modLoader == Any || modLoader == Forge || modLoader == Fabric;
    }

    inline QString getModLoaderString(ModLoaderType modLoader) const{
        switch(modLoader){
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
};
