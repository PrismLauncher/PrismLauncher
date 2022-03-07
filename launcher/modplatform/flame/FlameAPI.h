#pragma once

#include "modplatform/helpers/NetworkModAPI.h"

class FlameAPI : public NetworkModAPI {
   private:
    inline QString getModSearchURL(SearchArgs& args) const
    {
        return QString(
                   "https://addons-ecs.forgesvc.net/api/v2/addon/search?"
                   "gameId=432&"
                   "categoryId=0&"
                   "sectionId=6&"

                   "index=%1&"
                   "pageSize=25&"
                   "searchFilter=%2&"
                   "sort=%3&"
                   "modLoaderType=%4&"
                   "gameVersion=%5")
            .arg(args.offset)
            .arg(args.search)
            .arg(args.sorting)
            .arg(args.mod_loader)
            .arg(args.version);
    };

    inline QString getVersionsURL(const QString& addonId) const
    {
        return QString("https://addons-ecs.forgesvc.net/api/v2/addon/%1/files").arg(addonId);
    };
};
