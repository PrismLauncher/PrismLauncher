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

#include "api/modrinth/ModrinthAPI.h"
#include "BuildConfig.h"
#include "Json.h"
#include "api/modrinth/ModrinthUtils.h"
#include "api/structures/HttpRequest.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

namespace API {

QList<SortingMethod> ModrinthAPI::getSortingMethods() const
{
    // https://docs.modrinth.com/api-spec/#tag/projects/operation/searchProjects
    return { { 1, "relevance", QObject::tr("Sort by Relevance") },
             { 2, "downloads", QObject::tr("Sort by Downloads") },
             { 3, "follows", QObject::tr("Sort by Follows") },
             { 4, "newest", QObject::tr("Sort by Newest") },
             { 5, "updated", QObject::tr("Sort by Last Updated") } };
}

Platform::Provider ModrinthAPI::provider() const
{
    return Platform::Provider::MODRINTH;
}

bool ModrinthAPI::validateModLoaders(Platform::ModLoaders loaders) const
{
    return loaders & (Platform::ModLoader::NeoForge | Platform::ModLoader::Forge | Platform::ModLoader::Fabric |
                      Platform::ModLoader::Quilt | Platform::ModLoader::LiteLoader);
}

std::unique_ptr<HttpRequest> ModrinthAPI::prepareSearchRequest(SearchArgs const& args) const
{
    QStringList get_arguments;
    get_arguments.append(QString("offset=%1").arg(args.offset));
    get_arguments.append(QString("limit=25"));
    if (args.search.has_value())
        get_arguments.append(QString("query=%1").arg(args.search.value()));
    if (args.sorting.has_value())
        get_arguments.append(QString("index=%1").arg(args.sorting.value().name));
    get_arguments.append(QString("facets=%1").arg(ModrinthUtils::createFacets(args)));

    auto url = BuildConfig.MODRINTH_PROD_URL + "/search?" + get_arguments.join('&');
    return HttpRequest::GET(url);
}

bool ModrinthAPI::handleSearchResponse(const QJsonDocument& doc, QList<Platform::Project::Ptr>& rsp) const
{
    auto hits = doc.object().value("hits").toArray();
    return handleGetProjectsResponse(QJsonDocument(hits), rsp);
}

std::unique_ptr<HttpRequest> ModrinthAPI::prepareGetProjectRequest(QString const& id) const
{
    return HttpRequest::GET(BuildConfig.MODRINTH_PROD_URL + "/project/" + id);
}

bool ModrinthAPI::handleGetProjectResponse(const QJsonDocument& doc, Platform::Project& rsp) const
{
    try {
        auto obj = Json::requireObject(doc);
        Modrinth::loadIndexedPack(rsp, obj);
        Modrinth::loadExtraPackData(rsp, obj);
    } catch (const JSONValidationError& e) {
        qDebug() << doc;
        qWarning() << "Error while reading " << Platform::ProviderUtils::name(provider()) << " resource info: " << e.cause();
        return false;
    }
    return true;
}

std::unique_ptr<HttpRequest> ModrinthAPI::prepareGetVersionsRequest(VersionSearchArgs const& args) const
{
    QStringList get_arguments;
    if (args.mcVersions.has_value())
        get_arguments.append(QString("game_versions=[%1]").arg(ModrinthUtils::getGameVersionsString(args.mcVersions.value())));
    if (args.loaders.has_value())
        get_arguments.append(QString("loaders=[\"%1\"]").arg(ModrinthUtils::getModLoaderStrings(args.loaders.value()).join("\",\"")));

    return HttpRequest::GET(QString("%1/project/%2/version%3%4")
                                .arg(BuildConfig.MODRINTH_PROD_URL, args.pack.projectId.toString(), get_arguments.isEmpty() ? "" : "?",
                                     get_arguments.join('&')));
}

std::unique_ptr<HttpRequest> ModrinthAPI::prepareGetDependencyRequest(DependencySearchArgs const& args) const
{
    auto addonId = args.dependency.projectId.toString();
    return args.dependency.version.length() != 0
               ? HttpRequest::GET(QString("%1/version/%2").arg(BuildConfig.MODRINTH_PROD_URL, args.dependency.version))
               : prepareGetVersionsRequest({ { addonId }, std::list<::Version>{ args.mcVersion }, args.loader });
}

bool ModrinthAPI::handleGetVersionsResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const
{
    try {
        auto arr = doc.array();

        for (auto versionIter : arr) {
            auto obj = versionIter.toObject();

            auto file = Modrinth::loadIndexedPackVersion(obj);
            if (!file.projectId.isValid())
                file.projectId = rsp.projectId;

            if (file.fileId.isValid() && !file.downloadUrl.isEmpty())  // Heuristic to check if the returned value is valid
                rsp.versions.append(file);
        }

        auto orderSortPredicate = [](const Platform::Version& a, const Platform::Version& b) -> bool {
            // dates are in RFC 3339 format
            return a.date > b.date;
        };
        std::sort(rsp.versions.begin(), rsp.versions.end(), orderSortPredicate);
    } catch (const JSONValidationError& e) {
        qDebug() << doc;
        qWarning() << "Error while reading " << debugName() << " resource version: " << e.cause();
        return false;
    }
    return true;
}

bool ModrinthAPI::handleGetDependencyResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const
{
    return handleGetVersionsResponse(doc.isObject() ? QJsonDocument(QJsonArray({ doc.object() })) : doc, rsp);
}

std::unique_ptr<HttpRequest> ModrinthAPI::prepareGetProjectsRequest(QStringList const& ids) const
{
    return HttpRequest::GET(BuildConfig.MODRINTH_PROD_URL + QString("/projects?ids=[\"%1\"]").arg(ids.join("\",\"")));
}

bool ModrinthAPI::handleGetProjectsResponse(const QJsonDocument& doc, QList<Platform::Project::Ptr>& rsp) const
{
    auto packs = doc.array();
    for (auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        Platform::Project::Ptr pack = std::make_shared<Platform::Project>();
        try {
            Modrinth::loadIndexedPack(*pack, packObj);
            rsp << pack;
        } catch (const JSONValidationError& e) {
            qWarning() << "Error while loading resource from " << debugName() << ": " << e.cause();
            continue;
        }
    }
    return true;
}

}  // namespace API