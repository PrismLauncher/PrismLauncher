#pragma once

#include <QString>
#include <QList>

namespace ModPlatform {
class ListModel;
}

class ModAPI {
   protected:
    using CallerType = ModPlatform::ListModel;

   public:
    virtual ~ModAPI() = default;

    // https://docs.curseforge.com/?http#tocS_ModLoaderType
    enum ModLoaderType { Any = 0, Forge = 1, Cauldron = 2, LiteLoader = 3, Fabric = 4 };

    struct SearchArgs {
        int offset;
        QString search;
        QString sorting;
        ModLoaderType mod_loader;
        QString version;
    };

    virtual void searchMods(CallerType* caller, SearchArgs&& args) const = 0;


    struct VersionSearchArgs {
        QString addonId;
        QList<QString> mcVersions;
        ModLoaderType loader;
    };

    virtual void getVersions(CallerType* caller, VersionSearchArgs&& args) const = 0;
};
