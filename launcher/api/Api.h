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

#include <QByteArray>
#include <QDebug>
#include <QHash>
#include <QList>
#include <QString>
#include <functional>
#include <memory>

#include "api/RPCSink.h"
#include "api/structures/Arguments.h"
#include "api/structures/HttpRequest.h"
#include "api/structures/Project.h"
#include "api/structures/Provider.h"
#include "api/structures/ResourceType.h"
#include "api/structures/SortingMethod.h"
#include "net/ApiDownload.h"
#include "net/ApiUpload.h"
#include "net/NetRequest.h"
#include "net/Sink.h"

namespace API {

inline Net::NetRequest::Ptr makeRequest(const std::unique_ptr<API::HttpRequest> req, Net::Sink* sink)
{
    if (!req)
        return nullptr;
    switch (req->method) {
        case API::HttpMethod::GET:
            return Net::ApiDownload::makeCustomSink(req->url, sink);
        case API::HttpMethod::POST:
            return Net::ApiUpload::makeCustomSink(req->url, sink, req->data);
        default:
            return nullptr;
    }
}

class ProviderAPI {
   public:
    // Non-copyable
    ProviderAPI(const ProviderAPI&) = delete;
    ProviderAPI& operator=(const ProviderAPI&) = delete;
    virtual ~ProviderAPI() = default;

    [[nodiscard]] virtual QList<SortingMethod> getSortingMethods() const = 0;
    [[nodiscard]] virtual Platform::Provider provider() const = 0;
    [[nodiscard]] virtual bool validateModLoaders(Platform::ModLoaders loaders) const = 0;
    [[nodiscard]] QString debugName() const { return Platform::ProviderUtils::name(provider()); }

   protected:
    ProviderAPI() = default;

    // APIs
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareSearchRequest(SearchArgs const& args) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetProjectRequest(QString const& id) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetProjectsRequest(QStringList const& ids) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetVersionsRequest(VersionSearchArgs const& args) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetDependencyRequest(DependencySearchArgs const& args) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetDescriptionRequest(QString const& id) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareMatchHashesRequest(MatchHashesArgs const& args) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetCategoriesRequest(Platform::ResourceType type) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetFileChangelogRequest(VersionArgs type) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetVersionRequest(VersionArgs const& args) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetMultipleVersionsRequest(QStringList const& ids) const = 0;
    [[nodiscard]] virtual std::unique_ptr<HttpRequest> prepareGetLatestVersionsRequest(GetLatestVersionsArgs const& ids) const = 0;

    // Parsers
    [[nodiscard]] virtual bool handleSearchResponse(const QJsonDocument& doc, QList<Platform::Project::Ptr>& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetProjectResponse(const QJsonDocument& doc, Platform::Project& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetProjectsResponse(const QJsonDocument& doc, QList<Platform::Project::Ptr>& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetVersionsResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetDependencyResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetDescriptionResponse(const QJsonDocument& doc, Platform::Project& rsp) const = 0;
    [[nodiscard]] virtual bool handleMatchHashesResponse(const QJsonDocument& doc, MatchHashesResponse& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetCategoriesResponse(const QJsonDocument& doc, CategoriesResponse& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetFileChangelogResponse(const QJsonDocument& doc, QString& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetVersionResponse(const QJsonDocument& doc, VersionResponse& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetMultipleVersionsResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const = 0;
    [[nodiscard]] virtual bool handleGetLatestVersionsResponse(const QJsonDocument& doc, GetLatestVersionsResponse& rsp) const = 0;
#define DEFINE_REQUEST_HANDLER(FUNC_NAME, ARG_TYPE, RSP_TYPE, PREPARE_REQUEST, PARSE_RESPONSE)                                         \
    [[nodiscard]] inline Net::NetRequest::Ptr make##FUNC_NAME##Request(ARG_TYPE const& args, std::shared_ptr<RSP_TYPE> response) const \
    {                                                                                                                                  \
        auto req = PREPARE_REQUEST(args);                                                                                              \
        auto bound = std::bind(&ProviderAPI::PARSE_RESPONSE, this, std::placeholders::_1, std::placeholders::_2);                      \
        auto sink = new API::RPCSink<RSP_TYPE>(bound, response);                                                                       \
        return makeRequest(std::move(req), sink);                                                                                      \
    }

   public:
    // API public interface
    DEFINE_REQUEST_HANDLER(Search, SearchArgs, QList<Platform::Project::Ptr>, prepareSearchRequest, handleSearchResponse)
    DEFINE_REQUEST_HANDLER(GetProject, QString, Platform::Project, prepareGetProjectRequest, handleGetProjectResponse)
    DEFINE_REQUEST_HANDLER(GetProjects, QStringList, QList<Platform::Project::Ptr>, prepareGetProjectsRequest, handleGetProjectsResponse)
    DEFINE_REQUEST_HANDLER(GetDescription, QString, Platform::Project, prepareGetDescriptionRequest, handleGetDescriptionResponse)
    DEFINE_REQUEST_HANDLER(GetVersions, VersionSearchArgs, VersionSearchResponse, prepareGetVersionsRequest, handleGetVersionsResponse)
    DEFINE_REQUEST_HANDLER(GetDependency,
                           DependencySearchArgs,
                           VersionSearchResponse,
                           prepareGetDependencyRequest,
                           handleGetDependencyResponse)
    DEFINE_REQUEST_HANDLER(MatchHashes, MatchHashesArgs, MatchHashesResponse, prepareMatchHashesRequest, handleMatchHashesResponse)
    DEFINE_REQUEST_HANDLER(GetCategories,
                           Platform::ResourceType,
                           CategoriesResponse,
                           prepareGetCategoriesRequest,
                           handleGetCategoriesResponse)
    DEFINE_REQUEST_HANDLER(GetFileChangelog, VersionArgs, QString, prepareGetFileChangelogRequest, handleGetFileChangelogResponse)
    DEFINE_REQUEST_HANDLER(GetVersion, VersionArgs, VersionResponse, prepareGetVersionRequest, handleGetVersionResponse)
    DEFINE_REQUEST_HANDLER(GetMultipleVersions,
                           QStringList,
                           VersionSearchResponse,
                           prepareGetMultipleVersionsRequest,
                           handleGetMultipleVersionsResponse)
    DEFINE_REQUEST_HANDLER(GetLatestVersions,
                           GetLatestVersionsArgs,
                           GetLatestVersionsResponse,
                           prepareGetLatestVersionsRequest,
                           handleGetLatestVersionsResponse)

   public:
    // Factory getter
    static ProviderAPI* get(const Platform::Provider& key)
    {
        auto it = getRegistry().find(key);
        if (it != getRegistry().end()) {
            return it->second.get();
        }
        return nullptr;
    }

   protected:
    static std::unordered_map<Platform::Provider, std::unique_ptr<ProviderAPI>>& getRegistry()
    {
        static std::unordered_map<Platform::Provider, std::unique_ptr<ProviderAPI>> registry;
        return registry;
    }
    static void registerClass(std::unique_ptr<ProviderAPI> api) { getRegistry().emplace(api->provider(), std::move(api)); }
};

}  // namespace API
