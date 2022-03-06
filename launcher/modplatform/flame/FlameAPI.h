#pragma once

#include "modplatform/ModAPI.h"

class FlameAPI : public ModAPI {
   public:
    
    inline QString getModSearchURL(int index, QString searchFilter, QString sort, ModLoaderType modLoader, QString version) const override 
    {
        return QString("https://addons-ecs.forgesvc.net/api/v2/addon/search?"
            "gameId=432&" "categoryId=0&" "sectionId=6&"

            "index=%1&"  "pageSize=25&"       "searchFilter=%2&"
            "sort=%3&"   "modLoaderType=%4&"  "gameVersion=%5")
                .arg(index)
                .arg(searchFilter)
                .arg(sort)
                .arg(modLoader)  
                .arg(version);
    };

    inline QString getVersionsURL(const QString& addonId) const override
    {
        return QString("https://addons-ecs.forgesvc.net/api/v2/addon/%1/files").arg(addonId);
    };

    inline QString getAuthorURL(const QString& name) const override { return ""; };
};
