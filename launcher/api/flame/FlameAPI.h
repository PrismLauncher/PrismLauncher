// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <memory>
#include "api/Api.h"

namespace API {

class FlameAPI : public ProviderAPI {
   public:
    static void registerClass() { ProviderAPI::registerClass(std::unique_ptr<FlameAPI>(new FlameAPI())); }
    virtual ~FlameAPI() = default;

    [[nodiscard]] virtual QList<SortingMethod> getSortingMethods() const;
    [[nodiscard]] virtual Platform::Provider provider() const;
    [[nodiscard]] virtual bool validateModLoaders(Platform::ModLoaders loaders) const;

   protected:
    FlameAPI() = default;

    // APIs
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareSearchRequest(SearchArgs const& args) const;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetProjectRequest(QString const& id) const;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetProjectsRequest(QStringList const& ids) const;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetVersionsRequest(VersionSearchArgs const& args) const;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetDependencyRequest(DependencySearchArgs const& args) const;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetDescriptionRequest(QString const& id) const;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareMatchHashesRequest(MatchHashesArgs const& args) const { return nullptr; }
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetCategoriesRequest(Platform::ResourceType type) const { return nullptr; }

    // Parsers
    [[nodiscard]] virtual bool handleSearchResponse(const QJsonDocument& doc, QList<Platform::Project::Ptr>& rsp) const;
    [[nodiscard]] virtual bool handleGetProjectResponse(const QJsonDocument& doc, Platform::Project& rsp) const;
    [[nodiscard]] virtual bool handleGetProjectsResponse(const QJsonDocument& doc, QList<Platform::Project::Ptr>& rsp) const;
    [[nodiscard]] virtual bool handleGetVersionsResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const;
    [[nodiscard]] virtual bool handleGetDependencyResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const;
    [[nodiscard]] virtual bool handleGetDescriptionResponse(const QJsonDocument& doc, Platform::Project& rsp) const;
    [[nodiscard]] virtual bool handleMatchHashesResponse(const QJsonDocument& doc, QList<Platform::Version>& rsp) const { return false; }
    [[nodiscard]] virtual bool handleGetCategoriesResponse(const QJsonDocument& doc, QList<Platform::Category>& rsp) const { return false; }
};

}  // namespace API