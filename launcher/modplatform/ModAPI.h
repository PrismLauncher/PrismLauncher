#pragma once

#include <QString>

class ModAPI {
    public:
        virtual ~ModAPI() = default;

        // https://docs.curseforge.com/?http#tocS_ModLoaderType
        enum ModLoaderType {
            Any         = 0,
            Forge       = 1,
            Cauldron    = 2,
            LiteLoader  = 3,
            Fabric      = 4
        };

        inline virtual QString getModSearchURL(int, QString, QString, ModLoaderType, QString) const { return ""; };
        inline virtual QString getVersionsURL(const QString& addonId) const { return ""; };
        inline virtual QString getAuthorURL(const QString& name) const { return ""; };
};
