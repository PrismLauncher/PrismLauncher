#pragma once

#include <QString>
#include <QList>

#include "Version.h"

namespace ModPlatform {
class ListModel;
}

class ModAPI {
   protected:
    using CallerType = ModPlatform::ListModel;

   public:
    virtual ~ModAPI() = default;

    // https://docs.curseforge.com/?http#tocS_ModLoaderType
    enum ModLoaderType { Unspecified = 0, Forge = 1, Cauldron = 2, LiteLoader = 3, Fabric = 4, Quilt = 5 };

    struct SearchArgs {
        int offset;
        QString search;
        QString sorting;
        ModLoaderType mod_loader;
        std::list<Version> versions;
    };

    virtual void searchMods(CallerType* caller, SearchArgs&& args) const = 0;


    struct VersionSearchArgs {
        QString addonId;
        std::list<Version> mcVersions;
        ModLoaderType loader;
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
            s += ver.toString();
            if(ver != mcVersions.back())
                s += ",";
        }
        return s;
    }
};
