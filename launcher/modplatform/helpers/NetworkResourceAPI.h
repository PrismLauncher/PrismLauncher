// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "modplatform/ResourceAPI.h"

class NetworkResourceAPI : public ResourceAPI {
   public:
    Task::Ptr searchProjects(SearchArgs&&, SearchCallbacks&&) const override;

    Task::Ptr getProject(QString addonId, QByteArray* response) const override;

    Task::Ptr getProjectInfo(ProjectInfoArgs&&, ProjectInfoCallbacks&&) const override;
    Task::Ptr getProjectVersions(VersionSearchArgs&&, VersionSearchCallbacks&&) const override;

   protected:
    [[nodiscard]] virtual auto getSearchURL(SearchArgs const& args) const -> std::optional<QString> = 0;
    [[nodiscard]] virtual auto getInfoURL(QString const& id) const -> std::optional<QString> = 0;
    [[nodiscard]] virtual auto getVersionsURL(VersionSearchArgs const& args) const -> std::optional<QString> = 0;
};
