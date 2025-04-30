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

#include "api/flame/FlameAPI.h"
#include <memory>
#include "BuildConfig.h"
#include "Json.h"
#include "api/flame/FlameUtils.h"
#include "api/structures/HttpRequest.h"
#include "modplatform/flame/FlameModIndex.h"

namespace API {

QList<SortingMethod> FlameAPI::getSortingMethods() const
{
    // https://docs.curseforge.com/?python#tocS_ModsSearchSortField
    return { { 1, "Featured", QObject::tr("Sort by Featured") },
             { 2, "Popularity", QObject::tr("Sort by Popularity") },
             { 3, "LastUpdated", QObject::tr("Sort by Last Updated") },
             { 4, "Name", QObject::tr("Sort by Name") },
             { 5, "Author", QObject::tr("Sort by Author") },
             { 6, "TotalDownloads", QObject::tr("Sort by Downloads") },
             { 7, "Category", QObject::tr("Sort by Category") },
             { 8, "GameVersion", QObject::tr("Sort by Game Version") } };
}

Platform::Provider FlameAPI::provider() const
{
    return Platform::Provider::FLAME;
}

bool FlameAPI::validateModLoaders(Platform::ModLoaders loaders) const
{
    return loaders &
           (Platform::ModLoader::NeoForge | Platform::ModLoader::Forge | Platform::ModLoader::Fabric | Platform::ModLoader::Quilt);
}

std::unique_ptr<HttpRequest> FlameAPI::prepareSearchRequest(API::SearchArgs const& args) const
{
    QStringList get_arguments;
    get_arguments.append(QString("classId=%1").arg(FlameUtils::getClassId(args.type)));
    get_arguments.append(QString("index=%1").arg(args.offset));
    get_arguments.append("pageSize=25");
    if (args.search.has_value())
        get_arguments.append(QString("searchFilter=%1").arg(args.search.value()));
    if (args.sorting.has_value())
        get_arguments.append(QString("sortField=%1").arg(args.sorting.value().index));
    get_arguments.append("sortOrder=desc");
    if (args.loaders.has_value() && args.loaders.value() != 0)
        get_arguments.append(QString("modLoaderTypes=%1").arg(FlameUtils::getModLoaderFilters(args.loaders.value())));
    if (args.categoryIds.has_value() && !args.categoryIds->empty())
        get_arguments.append(QString("categoryIds=[%1]").arg(args.categoryIds->join(",")));

    if (args.versions.has_value() && !args.versions.value().empty())
        get_arguments.append(QString("gameVersion=%1").arg(args.versions.value().front().toString()));

    auto url = BuildConfig.FLAME_BASE_URL + "/mods/search?gameId=432&" + get_arguments.join('&');
    return HttpRequest::GET(url);
}

bool FlameAPI::handleSearchResponse(const QJsonDocument& doc, QList<Platform::Project::Ptr>& rsp) const
{
    auto packs = Json::ensureArray(doc.object(), "data");

    for (auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        Platform::Project::Ptr pack = std::make_shared<Platform::Project>();
        try {
            FlameMod::loadIndexedPack(*pack, packObj);
            rsp << pack;
        } catch (const JSONValidationError& e) {
            qWarning() << "Error while loading resource from " << debugName() << ": " << e.cause();
            continue;
        }
    }
    return true;
}

std::unique_ptr<HttpRequest> FlameAPI::prepareGetProjectRequest(QString const& id) const
{
    return HttpRequest::GET(QString(BuildConfig.FLAME_BASE_URL + "/mods/%1").arg(id));
}

bool FlameAPI::handleGetProjectResponse(const QJsonDocument& doc, Platform::Project& rsp) const
{
    auto obj = Json::requireObject(doc);
    obj = Json::requireObject(obj, "data");
    FlameMod::loadIndexedPack(rsp, obj);
    return true;
}

bool FlameAPI::handleGetDescriptionResponse(const QJsonDocument& doc, Platform::Project& rsp) const
{
    rsp.extraData.body = Json::ensureString(doc.object(), "data");
    rsp.extraDataLoaded = !rsp.extraData.issuesUrl.isEmpty() || !rsp.extraData.sourceUrl.isEmpty() || !rsp.extraData.wikiUrl.isEmpty() ||
                          !rsp.extraData.body.isEmpty();
    return true;
}
std::unique_ptr<HttpRequest> FlameAPI::prepareGetDescriptionRequest(QString const& id) const
{
    return HttpRequest::GET(QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/description").arg(id));
}

std::unique_ptr<HttpRequest> FlameAPI::prepareGetVersionsRequest(VersionSearchArgs const& args) const
{
    auto addonId = args.pack.projectId.toString();
    QString url = QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files?pageSize=10000").arg(addonId);

    if (args.mcVersions.has_value())
        url += QString("&gameVersion=%1").arg(args.mcVersions.value().front().toString());

    if (args.loaders.has_value() && Platform::ModloaderUtils::hasSingleSelected(args.loaders.value())) {
        int mappedModLoader = FlameUtils::getMappedModLoader(static_cast<Platform::ModLoader>(static_cast<int>(args.loaders.value())));
        url += QString("&modLoaderType=%1").arg(mappedModLoader);
    }
    return HttpRequest::GET(url);
}

std::unique_ptr<HttpRequest> FlameAPI::prepareGetDependencyRequest(DependencySearchArgs const& args) const
{
    auto addonId = args.dependency.projectId.toString();
    return prepareGetVersionsRequest({ { addonId }, std::list<::Version>{ args.mcVersion }, args.loader });
}

bool FlameAPI::handleGetVersionsResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const
{
    auto arr = Json::ensureArray(doc.object(), "data");

    for (auto versionIter : arr) {
        auto obj = versionIter.toObject();

        auto file = FlameUtils::loadIndexedPackVersion(obj, rsp.resourceType);
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
    return true;
}

bool FlameAPI::handleGetDependencyResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const
{
    return handleGetVersionsResponse(doc, rsp);
}

std::unique_ptr<HttpRequest> FlameAPI::prepareGetProjectsRequest(QStringList const& ids) const
{
    QJsonObject body;
    Json::writeStringList(body, "modIds", ids);
    return HttpRequest::POST(QString(BuildConfig.FLAME_BASE_URL + "/mods"), QJsonDocument(body).toJson());
}

bool FlameAPI::handleGetProjectsResponse(const QJsonDocument& doc, QList<Platform::Project::Ptr>& rsp) const
{
    return handleSearchResponse(doc, rsp);
}

std::unique_ptr<HttpRequest> FlameAPI::prepareGetCategoriesRequest(Platform::ResourceType type) const
{
    return HttpRequest::GET(QString(BuildConfig.FLAME_BASE_URL + "/categories?gameId=432&classId=%1").arg(FlameUtils::getClassId(type)));
}

bool FlameAPI::handleGetCategoriesResponse(const QJsonDocument& doc, CategoriesResponse& rsp) const
{
    auto obj = Json::requireObject(doc);
    auto arr = Json::requireArray(obj, "data");

    for (auto val : arr) {
        auto cat = Json::requireObject(val);
        auto id = Json::requireInteger(cat, "id");
        auto name = Json::requireString(cat, "name");
        rsp.categories.push_back({ name, QString::number(id) });
    }
    return true;
}

std::unique_ptr<HttpRequest> FlameAPI::prepareMatchHashesRequest(MatchHashesArgs const& args) const
{
    QJsonObject body;
    Json::writeStringList(body, "fingerprints", args.hashes);
    return HttpRequest::POST(QString(BuildConfig.FLAME_BASE_URL + "/fingerprints"), QJsonDocument(body).toJson());
}

bool FlameAPI::handleMatchHashesResponse(const QJsonDocument& doc, MatchHashesResponse& rsp) const
{
    auto doc_obj = Json::requireObject(doc);
    auto data_obj = Json::requireObject(doc_obj, "data");
    auto data_arr = Json::requireArray(data_obj, "exactMatches");

    if (data_arr.isEmpty()) {
        qWarning() << "No matches found for fingerprint search!";
        return true;
    }

    for (auto match : data_arr) {
        auto match_obj = Json::ensureObject(match, {});
        auto file_obj = Json::ensureObject(match_obj, "file", {});

        if (match_obj.isEmpty() || file_obj.isEmpty()) {
            qWarning() << "Fingerprint match is empty!";
            continue;
        }

        auto fingerprint = QString::number(Json::ensureVariant(file_obj, "fileFingerprint").toUInt());
        rsp.insert(fingerprint, FlameMod::loadIndexedPackVersion(file_obj));
    }
    return true;
}
std::unique_ptr<HttpRequest> FlameAPI::prepareGetFileChangelogRequest(VersionArgs args) const
{
    return HttpRequest::GET(
        QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files/%2/changelog").arg(args.projectId.toString(), args.fileId.toString()));
}

bool FlameAPI::handleGetFileChangelogResponse(const QJsonDocument& doc, QString& rsp) const
{
    rsp = Json::ensureString(doc.object(), "data");
    return true;
}

std::unique_ptr<HttpRequest> FlameAPI::prepareGetVersionRequest(VersionArgs const& args) const
{
    return HttpRequest::GET(
        QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files/%2").arg(args.projectId.toString(), args.fileId.toString()));
}

bool FlameAPI::handleGetVersionResponse(const QJsonDocument& doc, VersionResponse& rsp) const
{
    auto obj = Json::requireObject(doc.object(), "data");

    rsp.version = FlameUtils::loadIndexedPackVersion(obj, rsp.resourceType);
    if (!rsp.version.projectId.isValid())
        rsp.version.projectId = rsp.projectId;

    return rsp.version.fileId.isValid();  // Heuristic to check if the returned value is valid
}

std::unique_ptr<HttpRequest> FlameAPI::prepareGetMultipleVersionsRequest(QStringList const& ids) const
{
    QJsonObject body;
    Json::writeStringList(body, "fileIds", ids);
    return HttpRequest::POST(QString(BuildConfig.FLAME_BASE_URL + "/mods/files"), QJsonDocument(body).toJson());
}

bool FlameAPI::handleGetMultipleVersionsResponse(const QJsonDocument& doc, VersionSearchResponse& rsp) const
{
    return handleGetVersionsResponse(doc, rsp);
}

}  // namespace API