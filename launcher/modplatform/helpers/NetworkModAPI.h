#pragma once

#include "modplatform/ModAPI.h"

class NetworkModAPI : public ModAPI {
   public:
    void searchMods(CallerType* caller, SearchArgs&& args) const override;
    void getVersions(CallerType* caller, VersionSearchArgs&& args) const override;

   protected:
    virtual auto getModSearchURL(SearchArgs& args) const -> QString = 0;
    virtual auto getVersionsURL(VersionSearchArgs& args) const -> QString = 0;
};
