#pragma once

#include "modplatform/ModAPI.h"

class NetworkModAPI : public ModAPI {
   public:
    void searchMods(CallerType* caller, SearchArgs&& args) const override;
    void getModInfo(ModPlatform::IndexedPack& pack, std::function<void(QJsonDocument&, ModPlatform::IndexedPack&)> callback) override;
    void getVersions(VersionSearchArgs&& args, std::function<void(QJsonDocument&, QString)> callback) const override;

    auto getProject(QString addonId, QByteArray* response) const -> NetJob* override;

   protected:
    virtual auto getModSearchURL(SearchArgs& args) const -> QString = 0;
    virtual auto getModInfoURL(QString& id) const -> QString = 0;
    virtual auto getVersionsURL(VersionSearchArgs& args) const -> QString = 0;
};
