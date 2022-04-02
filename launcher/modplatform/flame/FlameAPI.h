#pragma once

#include "modplatform/helpers/NetworkModAPI.h"

class FlameAPI : public NetworkModAPI {
   private:
    inline auto getModSearchURL(SearchArgs& args) const -> QString override
    {
        auto gameVersionStr = args.versions.size() != 0 ? QString("gameVersion=%1").arg(args.versions.front().toString()) : QString();

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
                   "%5")
            .arg(args.offset)
            .arg(args.search)
            .arg(args.sorting)
            .arg(args.mod_loader)
            .arg(gameVersionStr);
    };

    inline auto getVersionsURL(VersionSearchArgs& args) const -> QString override
    {
        return QString("https://addons-ecs.forgesvc.net/api/v2/addon/%1/files").arg(args.addonId);
    };
};
