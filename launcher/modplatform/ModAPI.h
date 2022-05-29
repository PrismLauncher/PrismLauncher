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
