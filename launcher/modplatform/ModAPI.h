#pragma once

#include <QString>

class ModAPI {
    public:
        virtual ~ModAPI() = default;

        inline virtual QString getModSearchURL(int, QString, QString, bool, QString) const { return ""; };
        inline virtual QString getVersionsURL(const QString& addonId) const { return ""; };
        inline virtual QString getAuthorURL(const QString& name) const { return ""; };
};
