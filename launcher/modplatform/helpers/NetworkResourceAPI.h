// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>
#include "modplatform/ResourceAPI.h"

class NetworkResourceAPI : public ResourceAPI {
   public:
    TaskV2::Ptr searchProjects(SearchArgs&&, SearchCallbacks&&) const override;

    TaskV2::Ptr getProject(QString addonId, std::shared_ptr<QByteArray> response) const override;

    TaskV2::Ptr getProjectInfo(ProjectInfoArgs&&, ProjectInfoCallbacks&&) const override;
    TaskV2::Ptr getProjectVersions(VersionSearchArgs&&, VersionSearchCallbacks&&) const override;
    TaskV2::Ptr getDependencyVersion(DependencySearchArgs&&, DependencySearchCallbacks&&) const override;

   protected:
    [[nodiscard]] virtual auto getSearchURL(SearchArgs const& args) const -> std::optional<QString> = 0;
    [[nodiscard]] virtual auto getInfoURL(QString const& id) const -> std::optional<QString> = 0;
    [[nodiscard]] virtual auto getVersionsURL(VersionSearchArgs const& args) const -> std::optional<QString> = 0;
    [[nodiscard]] virtual auto getDependencyURL(DependencySearchArgs const& args) const -> std::optional<QString> = 0;
};
