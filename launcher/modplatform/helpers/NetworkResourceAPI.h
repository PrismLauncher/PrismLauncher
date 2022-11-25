#pragma once

#include "modplatform/ResourceAPI.h"

class NetworkResourceAPI : public ResourceAPI {
   public:
    NetJob::Ptr searchProjects(SearchArgs&&, SearchCallbacks&&) const override;

    NetJob::Ptr getProject(QString addonId, QByteArray* response) const override;

    NetJob::Ptr getProjectInfo(ProjectInfoArgs&&, ProjectInfoCallbacks&&) const override;
    NetJob::Ptr getProjectVersions(VersionSearchArgs&&, VersionSearchCallbacks&&) const override;

   protected:
    [[nodiscard]] virtual auto getSearchURL(SearchArgs const& args) const -> std::optional<QString> = 0;
    [[nodiscard]] virtual auto getInfoURL(QString const& id) const -> std::optional<QString> = 0;
    [[nodiscard]] virtual auto getVersionsURL(VersionSearchArgs const& args) const -> std::optional<QString> = 0;
};
