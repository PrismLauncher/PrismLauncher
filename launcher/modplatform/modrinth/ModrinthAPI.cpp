// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModrinthAPI.h"
#include <array>

#include "Application.h"
#include "Json.h"
#include "modplatform/ResourceType.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"

namespace {

QStringList getModLoaderStrings(const ModPlatform::ModLoaderTypes types)
{
    QStringList l;
    for (auto loader : { ModPlatform::NeoForge, ModPlatform::Forge, ModPlatform::Fabric, ModPlatform::Quilt, ModPlatform::LiteLoader,
                         ModPlatform::DataPack, ModPlatform::Babric, ModPlatform::BTA, ModPlatform::LegacyFabric, ModPlatform::Ornithe,
                         ModPlatform::Rift }) {
        if ((types & loader) != 0U) {
            l << getModLoaderAsString(loader);
        }
    }
    return l;
}

QString getModLoaderFilters(ModPlatform::ModLoaderTypes types)
{
    QStringList l;
    for (const auto& loader : getModLoaderStrings(types)) {
        l << QString("\"categories:%1\"").arg(loader);
    }
    return l.join(',');
}

QString mapMCVersionToModrinth(const Version& v)
{
    static const QString s_preString = " Pre-Release ";
    auto verStr = v.toString();

    if (verStr.contains(s_preString)) {
        verStr.replace(s_preString, "-pre");
    }
    verStr.replace(" ", "-");
    return verStr;
}

QString getGameVersionsString(const std::vector<Version>& mcVersions)
{
    QString s;
    for (const auto& ver : mcVersions) {
        s += QString("\"%1\",").arg(mapMCVersionToModrinth(ver));
    }
    s.remove(s.length() - 1, 1);  // remove last comma
    return s;
}

QString getGameVersionsArray(const std::vector<Version>& mcVersions)
{
    QString s;
    for (const auto& ver : mcVersions) {
        s += QString(R"("versions:%1",)").arg(mapMCVersionToModrinth(ver));
    }
    s.remove(s.length() - 1, 1);  // remove last comma
    return s.isEmpty() ? QString() : s;
}

QString getCategoriesFilters(const QStringList& categories)
{
    QStringList l;
    for (const auto& cat : categories) {
        l << QString("\"categories:%1\"").arg(cat);
    }
    return l.join(',');
}

QString getSideFilters(ModPlatform::SideType side)
{
    switch (side.value()) {
        case ModPlatform::SideType::ClientSide:
            return {
                R"("environment:client_only","environment:client_only_server_optional","environment:singleplayer_only","environment:client_or_server","environment:client_or_server_prefers_both")"
            };
        case ModPlatform::SideTypeValue::ServerSide:
            return {
                R"("environment:server_only","environment:server_only_client_optional","environment:dedicated_server_only","environment:client_or_server","environment:client_or_server_prefers_both")"
            };
        case ModPlatform::SideTypeValue::UniversalSide:
            return { R"("environment:client_and_server","client_or_server_prefers_both")" };
        case ModPlatform::SideTypeValue::NoSide:
        // fallthrough
        default:
            return {};
    }
}

QString createFacets(const ResourceAPI::SearchArgs& args)
{
    QStringList facetsList;

    if (args.loaders.has_value() && args.loaders.value() != 0) {
        facetsList.append(QString("[%1]").arg(getModLoaderFilters(args.loaders.value())));
    }
    if (args.versions.has_value() && !args.versions.value().empty()) {
        facetsList.append(QString("[%1]").arg(getGameVersionsArray(args.versions.value())));
    }
    if (args.side.has_value()) {
        auto side = getSideFilters(args.side.value());
        if (!side.isEmpty()) {
            facetsList.append(QString("[%1]").arg(side));
        }
    }
    if (args.categoryIds.has_value() && !args.categoryIds->empty()) {
        facetsList.append(QString("[%1]").arg(getCategoriesFilters(args.categoryIds.value())));
    }
    if (!args.excludeDisclosureTypes.empty()) {
        for (const auto& d : args.excludeDisclosureTypes) {
            facetsList.append(QString("[\"disclosure_types!=%1\"]").arg(d.toString()));
        }
    }
    if (args.openSource) {
        facetsList.append("[\"open_source:true\"]");
    }

    facetsList.append(QString("[\"project_type:%1\"]").arg(Modrinth::Parse::resourceTypeParameter(args.type)));

    return QString("[%1]").arg(facetsList.join(','));
}
}  // namespace

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::currentVersion(const QString& hash, const QString& hashFormat)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCurrentVersion"), APPLICATION->network());

    auto [action, response] =
        Net::ApiRequest::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1?algorithm=%2").arg(hash, hashFormat));
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::currentVersions(const QStringList& hashes, const QString& hashFormat)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCurrentVersions"), APPLICATION->network());

    QJsonObject bodyObj;

    Json::writeStringList(bodyObj, "hashes", hashes);
    Json::writeString(bodyObj, "algorithm", hashFormat);

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();

    auto [action, response] = Net::ApiRequest::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files"), bodyRaw);
    netJob->addNetAction(action);
    netJob->setAskRetry(false);
    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::latestVersion(const QString& hash,
                                                             const QString& hashFormat,
                                                             std::optional<std::vector<Version>> mcVersions,
                                                             std::optional<ModPlatform::ModLoaderTypes> loaders) const
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetLatestVersion"), APPLICATION->network());

    QJsonObject bodyObj;

    if (loaders.has_value()) {
        Json::writeStringList(bodyObj, "loaders", getModLoaderStrings(loaders.value()));
    }

    if (mcVersions.has_value()) {
        QStringList gameVersions;
        for (auto& ver : mcVersions.value()) {
            gameVersions.append(mapMCVersionToModrinth(ver));
        }
        Json::writeStringList(bodyObj, "game_versions", gameVersions);
    }

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();

    auto [action, response] = Net::ApiRequest::makeByteArray(
        QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1/update?algorithm=%2").arg(hash, hashFormat), bodyRaw);
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::latestVersions(const QStringList& hashes,
                                                              const QString& hashFormat,
                                                              std::optional<std::vector<Version>> mcVersions,
                                                              std::optional<ModPlatform::ModLoaderTypes> loaders) const
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetLatestVersions"), APPLICATION->network());

    QJsonObject bodyObj;

    Json::writeStringList(bodyObj, "hashes", hashes);
    Json::writeString(bodyObj, "algorithm", hashFormat);

    if (loaders.has_value()) {
        Json::writeStringList(bodyObj, "loaders", getModLoaderStrings(loaders.value()));
    }

    if (mcVersions.has_value()) {
        QStringList gameVersions;
        for (auto& ver : mcVersions.value()) {
            gameVersions.append(mapMCVersionToModrinth(ver));
        }
        Json::writeStringList(bodyObj, "game_versions", gameVersions);
    }

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();
    auto [action, response] = Net::ApiRequest::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files/update"), bodyRaw);
    netJob->addNetAction(action);

    return { netJob, response };
}

QList<ResourceAPI::SortingMethod> ModrinthAPI::getSortingMethods() const
{
    // https://docs.modrinth.com/api-spec/#tag/projects/operation/searchProjects
    return { { .index = 1, .name = "relevance", .readableName = QObject::tr("Sort by Relevance") },
             { .index = 2, .name = "downloads", .readableName = QObject::tr("Sort by Downloads") },
             { .index = 3, .name = "follows", .readableName = QObject::tr("Sort by Follows") },
             { .index = 4, .name = "newest", .readableName = QObject::tr("Sort by Newest") },
             { .index = 5, .name = "updated", .readableName = QObject::tr("Sort by Last Updated") } };
}
bool ModrinthAPI::validateModLoaders(ModPlatform::ModLoaderTypes loaders)
{
    return loaders.testAnyFlags(ModPlatform::NeoForge | ModPlatform::Forge | ModPlatform::Fabric | ModPlatform::Quilt |
                                ModPlatform::LiteLoader | ModPlatform::DataPack | ModPlatform::Babric | ModPlatform::BTA |
                                ModPlatform::LegacyFabric | ModPlatform::Ornithe | ModPlatform::Rift);
}

auto ModrinthAPI::getVersionsURL(const VersionSearchArgs& args) const -> std::optional<QString>
{
    QStringList getArguments;
    if (args.mcVersions.has_value()) {
        getArguments.append(QString("game_versions=[%1]").arg(getGameVersionsString(args.mcVersions.value())));
    }
    if (args.loaders.has_value()) {
        getArguments.append(QString("loaders=[\"%1\"]").arg(getModLoaderStrings(args.loaders.value()).join("\",\"")));
    }
    getArguments.append(QString("include_changelog=%1").arg(args.includeChangelog ? "true" : "false"));

    return QString("%1/project/%2/version%3%4")
        .arg(BuildConfig.MODRINTH_PROD_URL, args.pack->addonId.toString(), getArguments.isEmpty() ? "" : "?", getArguments.join('&'));
}

std::optional<QString> ModrinthAPI::getDependencyURL(const DependencySearchArgs& args) const
{
    return args.dependency.version.length() != 0
               ? QString("%1/version/%2").arg(BuildConfig.MODRINTH_PROD_URL, args.dependency.version)
               : QString(R"(%1/project/%2/version?game_versions=["%3"]&loaders=["%4"]&include_changelog=%5)")
                     .arg(BuildConfig.MODRINTH_PROD_URL)
                     .arg(args.dependency.addonId.toString())
                     .arg(mapMCVersionToModrinth(args.mcVersion))
                     .arg(getModLoaderStrings(args.loader).join("\",\""))
                     .arg(args.includeChangelog ? "true" : "false");
}

Net::RPC::Spec<ModPlatform::IndexedPack> ModrinthAPI::getProject(const QString& id) const
{
    // https://docs.modrinth.com/api/operations/getproject/
    return { { .url = QUrl(BuildConfig.MODRINTH_PROD_URL + "/project/" + id) },
             [id](const auto& response) -> Result<ModPlatform::IndexedPack> {
                 ModPlatform::IndexedPack pack = { .addonId = id };
                 TRY(Json::requireObject(response).and_then([&pack](const auto& v) { return Modrinth::Parse::loadIndexedPack(pack, v); }))
                 return pack;
             } };
}

Net::RPC::Spec<QList<ModPlatform::IndexedPack>> ModrinthAPI::getProjects(const QStringList& addonIds) const
{
    // https://docs.modrinth.com/api/operations/getprojects/
    auto url = getMultipleModInfoURL(addonIds);

    return { { .url = url }, [](const auto& response) -> Result<QList<ModPlatform::IndexedPack>> {
                QList<ModPlatform::IndexedPack> newList;
                TRY_INTO(auto doc,
                         Json::requireDocument(response, "ResourceAPI").and_then([](const auto& v) { return Json::requireArray(v); }))

                for (auto packRaw : doc) {
                    auto packObj = packRaw.toObject();

                    ModPlatform::IndexedPack pack;
                    TRY(Modrinth::Parse::loadIndexedPack(pack, packObj))
                    newList << pack;
                }
                return newList;
            } };
}

Net::RPC::Spec<QList<ModPlatform::IndexedPack>> ModrinthAPI::searchProjects(const SearchArgs& args) const
{
    // https://docs.modrinth.com/api/operations/searchprojects/
    auto url = searchProjectsURL(args);
    return { { .url = url }, [](const auto& response) -> Result<QList<ModPlatform::IndexedPack>> {
                QList<ModPlatform::IndexedPack> newList;
                TRY_INTO(auto doc, Json::requireDocument(response, "ResourceAPI")
                                       .and_then([](const auto& v) { return Json::requireObject(v); })
                                       .and_then([](const auto& v) { return Json::requireArray(v, "hits"); }))

                for (auto packRaw : doc) {
                    auto packObj = packRaw.toObject();

                    ModPlatform::IndexedPack pack;
                    TRY(Modrinth::Parse::loadIndexedPack(pack, packObj))
                    newList << pack;
                }
                return newList;
            } };
}

QUrl ModrinthAPI::searchProjectsURL(const SearchArgs& args)
{
    if (args.loaders.has_value() && args.loaders.value() != 0) {
        if (!validateModLoaders(args.loaders.value())) {
            qWarning() << "Modrinth - or our interface - does not support any the provided mod loaders!";
            return {};
        }
    }

    QStringList getArguments;
    getArguments.append(QString("offset=%1").arg(args.offset));
    getArguments.append(QString("limit=25"));
    if (args.search.has_value()) {
        getArguments.append(QString("query=%1").arg(args.search.value()));
    }
    if (args.sorting.has_value()) {
        getArguments.append(QString("index=%1").arg(args.sorting.value().name));
    }
    getArguments.append(QString("facets=%1").arg(createFacets(args)));

    return BuildConfig.MODRINTH_PROD_URL + "/search?" + getArguments.join('&');
};

Net::RPC::Spec<QList<ModPlatform::Category>> ModrinthAPI::getCategories(ModPlatform::ResourceType type) const
{  // https://docs.modrinth.com/api/operations/categorylist/
    auto projectType = Modrinth::Parse::resourceTypeParameter(type);
    return { { .url = BuildConfig.MODRINTH_PROD_URL + "/tag/category" },
             [projectType](const auto& response) -> Result<QList<ModPlatform::Category>> {
                 QList<ModPlatform::Category> categories;
                 TRY_INTO(const auto& doc, Json::requireArray(response))

                 for (auto val : doc) {
                     TRY_INTO(const auto& cat, Json::requireObject(val))
                     TRY_INTO(const auto& name, Json::requireString(cat, "name"))
                     if (cat["project_type"].toString() == projectType) {
                         categories.push_back({ .name = name, .id = name });
                     }
                 }
                 return categories;
             } };
}
