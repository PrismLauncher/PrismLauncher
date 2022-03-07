#pragma once

#include <QJsonDocument>
#include <QString>

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

    inline virtual void searchMods(CallerType* caller, SearchArgs&& args) const {};
    inline virtual void getVersions(CallerType* caller, const QString& addonId, const QString& debugName = "") const {};
};
