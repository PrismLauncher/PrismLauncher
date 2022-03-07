#pragma once

#include "modplatform/ModAPI.h"

class NetworkModAPI : public ModAPI {
   public:
    void searchMods(CallerType* caller, SearchArgs&& args) const override;
    void getVersions(CallerType* caller, const QString& addonId) const override;

   protected:
    virtual QString getModSearchURL(SearchArgs& args) const = 0;
    virtual QString getVersionsURL(const QString& addonId) const = 0;
};
