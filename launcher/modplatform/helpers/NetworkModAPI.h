#pragma once

#include "modplatform/ModAPI.h"

class NetworkModAPI : public ModAPI {
   public:
    void searchMods(CallerType* caller, SearchArgs&& args) const override;
    void getVersions(CallerType* caller, const QString& addonId) const override;

   protected:
    virtual auto getModSearchURL(SearchArgs& args) const -> QString = 0;
    virtual auto getVersionsURL(const QString& addonId) const -> QString = 0;
};
