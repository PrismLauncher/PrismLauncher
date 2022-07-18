#pragma once

#include "modplatform/ModAPI.h"

class NetworkModAPI : public ModAPI {
   public:
    void searchMods(CallerType* caller, SearchArgs&& args) const override;
    void getModInfo(CallerType* caller, ModPlatform::IndexedPack& pack) override;
    void getVersions(CallerType* caller, VersionSearchArgs&& args) const override;

    auto getProject(QString addonId, QByteArray* response) const -> NetJob* override;

   protected:
    virtual auto getModSearchURL(SearchArgs& args) const -> QString = 0;
    virtual auto getModInfoURL(QString& id) const -> QString = 0;
    virtual auto getVersionsURL(VersionSearchArgs& args) const -> QString = 0;
};
