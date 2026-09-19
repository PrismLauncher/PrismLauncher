// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "FlameAPI.h"
#include <optional>
#include "BuildConfig.h"

#include "Application.h"
#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlamePackIndex.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"
#include "net/Request.h"

namespace {

int getMappedModLoader(ModPlatform::ModLoaderType loaders)
{
    // https://docs.curseforge.com/?http#tocS_ModLoaderType
    switch (loaders) {
        case ModPlatform::Forge:
            return 1;
        case ModPlatform::Cauldron:
            return 2;
        case ModPlatform::LiteLoader:
            return 3;
        case ModPlatform::Fabric:
            return 4;
        case ModPlatform::Quilt:
            return 5;
        case ModPlatform::NeoForge:
            return 6;
        case ModPlatform::DataPack:
        case ModPlatform::Babric:
        case ModPlatform::BTA:
        case ModPlatform::LegacyFabric:
        case ModPlatform::Ornithe:
        case ModPlatform::Rift:
        case ModPlatform::None:
            break;  // not supported
    }
    return 0;
}

QStringList getModLoaderStrings(const ModPlatform::ModLoaderTypes types)
{
    QStringList l;
    for (auto loader : { ModPlatform::NeoForge, ModPlatform::Forge, ModPlatform::Fabric, ModPlatform::Quilt }) {
        if (types.testAnyFlag(loader)) {
            l << QString::number(getMappedModLoader(loader));
        }
    }
    return l;
}

QString getModLoaderFilters(ModPlatform::ModLoaderTypes types)
{
    return "[" + getModLoaderStrings(types).join(',') + "]";
}

}  // namespace

QList<ResourceAPI::SortingMethod> FlameAPI::getSortingMethods() const
{
    // https://docs.curseforge.com/?python#tocS_ModsSearchSortField
    return { { .index = 1, .name = "Featured", .readableName = QObject::tr("Sort by Featured") },
             { .index = 2, .name = "Popularity", .readableName = QObject::tr("Sort by Popularity") },
             { .index = 3, .name = "LastUpdated", .readableName = QObject::tr("Sort by Last Updated") },
             { .index = 4, .name = "Name", .readableName = QObject::tr("Sort by Name") },
             { .index = 5, .name = "Author", .readableName = QObject::tr("Sort by Author") },
             { .index = 6, .name = "TotalDownloads", .readableName = QObject::tr("Sort by Downloads") },
             { .index = 7, .name = "Category", .readableName = QObject::tr("Sort by Category") },
             { .index = 8, .name = "GameVersion", .readableName = QObject::tr("Sort by Game Version") } };
}

bool FlameAPI::validateModLoaders(ModPlatform::ModLoaderTypes loaders)
{
    return loaders.testAnyFlags(ModPlatform::NeoForge | ModPlatform::Forge | ModPlatform::Fabric | ModPlatform::Quilt);
}

Net::RPC::Spec<ModPlatform::IndexedPack> FlameAPI::getProject(const QString& id) const
{
    // https://docs.curseforge.com/rest-api/#get-mod
    return { { .url = QUrl(BuildConfig.FLAME_BASE_URL + "/mods/" + id) }, [id](const auto& response) -> Result<ModPlatform::IndexedPack> {
                ModPlatform::IndexedPack pack = { .addonId = id };
                TRY(Json::requireObject(response)
                        .and_then([](const auto& v) { return Json::requireObject(v, "data"); })
                        .and_then([&pack](const auto& v) { return Flame::Parse::loadIndexedPack(pack, v); }))
                return pack;
            } };
}

std::optional<Net::RPC::Spec<bool>> FlameAPI::getProjectExtra(ModPlatform::IndexedPack& pack) const
{
    // https://docs.curseforge.com/rest-api/#get-mod-description
    auto url = QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/description").arg(pack.addonId.toString());
    return { { { .url = QUrl(url) }, [&pack](const auto& response) -> Result<bool> {
                  TRY_INTO(auto doc, Json::requireDocument(response, "Flame::ModDescription"))
                  pack.extraData.body = doc.object()["data"].toString();

                  if (!pack.extraData.issuesUrl.isEmpty() || !pack.extraData.sourceUrl.isEmpty() || !pack.extraData.wikiUrl.isEmpty() ||
                      !pack.extraData.body.isEmpty()) {
                      pack.extraDataLoaded = true;
                  }
                  return true;
              } } };
}

Net::RPC::Spec<QList<ModPlatform::IndexedPack>> FlameAPI::searchProjects(const SearchArgs& args) const
{
    // https://docs.curseforge.com/rest-api/#search-mods
    auto url = searchProjectsURL(args);
    return { { .url = url }, Flame::Parse::parseProjectList };
}

Net::RPC::Spec<QList<ModPlatform::IndexedPack>> FlameAPI::getProjects(const QStringList& addonIds) const
{
    // https://docs.curseforge.com/rest-api/#get-mods
    QJsonObject bodyObj;
    bodyObj["modIds"] = Json::toJsonArray(addonIds);
    auto body = QJsonDocument(bodyObj).toJson();

    auto url = BuildConfig.FLAME_BASE_URL + "/mods";

    return { { .method = Net::HttpMethod::Post, .url = url, .data = body }, Flame::Parse::parseProjectList };
}

Net::RPC::Spec<QList<ModPlatform::Category>> FlameAPI::getCategories(ModPlatform::ResourceType type) const
{  // https://docs.curseforge.com/rest-api/#get-categories
    auto url = QString(BuildConfig.FLAME_BASE_URL + "/categories?gameId=432&classId=%1").arg(Flame::Parse::getClassId(type));
    return { { .url = url }, [](const auto& response) -> Result<QList<ModPlatform::Category>> {
                QList<ModPlatform::Category> categories;
                TRY_INTO(const auto& doc,
                         Json::requireObject(response).and_then([](const auto& v) { return Json::requireArray(v, "data"); }))

                for (auto val : doc) {
                    TRY_INTO(const auto& cat, Json::requireObject(val))
                    TRY_INTO(const auto& id, Json::requireInteger(cat, "id"))
                    TRY_INTO(const auto& name, Json::requireString(cat, "name"))
                    categories.push_back({ .name = name, .id = QString::number(id) });
                }
                return categories;
            } };
}
Net::RPC::Spec<QList<ModPlatform::IndexedVersion>> FlameAPI::getVersions(const VersionSearchArgs& args) const
{
    // https://docs.curseforge.com/rest-api/#get-mod-files
    auto url = getVersionsURL(args);
    return { { .url = url }, [args](const auto& response) -> Result<QList<ModPlatform::IndexedVersion>> {
                TRY_INTO(auto doc, Json::requireObject(response, "ResourceAPI::getVersions").and_then([](const auto& v) {
                    return Json::requireArray(v, "data");
                }))

                TRY_INTO(args.pack->versions,
                         Flame::Parse::loadIndexedPackVersions(doc, args.pack->addonId.toString(), args.pack->resourceType))
                args.pack->versionsLoaded = true;

                return args.pack->versions;
            } };
}

Net::RPC::Spec<ModPlatform::IndexedVersion> FlameAPI::getVersion(const QString& id, const QString& versionId) const
{
    // https://docs.curseforge.com/rest-api/#get-mod-file
    auto url = QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files/%2").arg(id, versionId);
    return { { .url = url }, [](const auto& response) -> Result<ModPlatform::IndexedVersion> {
                return Json::requireObject(response, "ResourceAPI::getVersions")
                    .and_then([](const auto& v) { return Json::requireObject(v, "data"); })
                    .and_then([](const auto& v) { return Flame::Parse::loadIndexedPackVersion(v); });
            } };
}

Net::RPC::Spec<QList<ModPlatform::IndexedVersion>> FlameAPI::getVersions(const QStringList& versionIds) const
{
    // https://docs.curseforge.com/rest-api/#get-files
    QJsonObject bodyObj;
    bodyObj["fileIds"] = Json::toJsonArray(versionIds);

    auto body = QJsonDocument(bodyObj).toJson();

    return { { .method = Net::HttpMethod::Post, .url = BuildConfig.FLAME_BASE_URL + "/mods/files", .data = body },
             [](const auto& response) -> Result<QList<ModPlatform::IndexedVersion>> {
                 TRY_INTO(auto doc, Json::requireObject(response, "ResourceAPI::getVersions").and_then([](const auto& v) {
                     return Json::requireArray(v, "data");
                 }))

                 return Flame::Parse::loadIndexedPackVersions(doc);
             } };
}

Net::RPC::Spec<QString> FlameAPI::getChangelog(const QString& id, const QString& fileId)
{
    // https://docs.curseforge.com/rest-api/#get-mod-file-changelog
    auto url = QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files/%2/changelog").arg(id, fileId);
    return { { .url = url }, [](const auto& response) -> Result<QString> {
                TRY_INTO(auto doc, Json::requireObject(response, "Flame::FileChangelog"))
                return doc["data"].toString();
            } };
}

std::pair<NetJob::Ptr, QString*> FlameAPI::getChangelogTask(const QString& id, const QString& fileId)
{
    auto spec = getChangelog(id, fileId);

    auto netJob = makeShared<NetJob>("Flame::Changelog", APPLICATION->network());

    auto [action, response] = Net::RPC::make<QString>(spec);
    netJob->addNetAction(action);

    return { netJob, response };
}

Net::RPC::Spec<QHash<QString, ModPlatform::IndexedVersion>> FlameAPI::matchFingerprints(const QList<uint>& fingerprints, bool onlyAvailable)
{
    // https://docs.curseforge.com/rest-api/#get-fingerprints-matches

    QJsonObject bodyObj;
    QJsonArray fingerprintsArr;
    for (const auto& fp : fingerprints) {
        fingerprintsArr.append(QString::number(fp));
    }

    bodyObj["fingerprints"] = fingerprintsArr;

    auto body = QJsonDocument(bodyObj).toJson();

    return { { .method = Net::HttpMethod::Post, .url = BuildConfig.FLAME_BASE_URL + "/fingerprints", .data = body },
             [onlyAvailable](const auto& response) -> Result<QHash<QString, ModPlatform::IndexedVersion>> {
                 TRY_INTO(auto doc, Json::requireObject(response, "matchFingerprints")
                                        .and_then([](const auto& v) { return Json::requireObject(v, "data"); })
                                        .and_then([](const auto& v) { return Json::requireArray(v, "exactMatches"); }))
                 QHash<QString, ModPlatform::IndexedVersion> matches;
                 for (auto entry : doc) {
                     TRY_INTO(auto file, Json::requireObject(entry, "file"))
                     if (onlyAvailable && file["isAvailable"].toBool()) {
                         continue;
                     }
                     auto fingerprint = QString::number(file["fileFingerprint"].toInteger());
                     TRY_INTO(matches[fingerprint], Flame::Parse::loadIndexedPackVersion(file))
                 }
                 return matches;
             } };
}

std::pair<NetJob::Ptr, QHash<QString, ModPlatform::IndexedVersion>*> FlameAPI::matchFingerprintsTask(const QList<uint>& fingerprints,
                                                                                                     bool onlyAvailable)
{
    auto spec = matchFingerprints(fingerprints, onlyAvailable);

    auto netJob = makeShared<NetJob>("Flame::matchFingerprints", APPLICATION->network());

    auto [action, response] = Net::RPC::make<QHash<QString, ModPlatform::IndexedVersion>>(spec);
    netJob->addNetAction(action);

    return { netJob, response };
}

QUrl FlameAPI::searchProjectsURL(const SearchArgs& args)
{
    QStringList getArguments;
    getArguments.append(QString("classId=%1").arg(Flame::Parse::getClassId(args.type)));
    getArguments.append(QString("index=%1").arg(args.offset));
    getArguments.append("pageSize=25");
    if (args.search.has_value()) {
        getArguments.append(QString("searchFilter=%1").arg(args.search.value()));
    }
    if (args.sorting.has_value()) {
        getArguments.append(QString("sortField=%1").arg(args.sorting.value().index));
    }
    getArguments.append("sortOrder=desc");
    if (args.loaders.has_value()) {
        ModPlatform::ModLoaderTypes loaders = args.loaders.value();
        loaders &= ~static_cast<std::uint16_t>(ModPlatform::ModLoaderType::DataPack);
        if (loaders != 0) {
            getArguments.append(QString("modLoaderTypes=%1").arg(getModLoaderFilters(loaders)));
        }
    }
    if (args.categoryIds.has_value() && !args.categoryIds->empty()) {
        getArguments.append(QString("categoryIds=[%1]").arg(args.categoryIds->join(",")));
    }

    if (args.versions.has_value() && !args.versions.value().empty()) {
        getArguments.append(QString("gameVersion=%1").arg(args.versions.value().front().toString()));
    }

    return BuildConfig.FLAME_BASE_URL + "/mods/search?gameId=432&" + getArguments.join('&');
}

QUrl FlameAPI::getVersionsURL(const VersionSearchArgs& args)
{
    auto addonId = args.pack->addonId.toString();
    QString url = QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files?pageSize=10000").arg(addonId);

    if (args.mcVersions.has_value()) {
        url += QString("&gameVersion=%1").arg(args.mcVersions.value().front().toString());
    }

    if (args.loaders.has_value() && args.loaders.value() != ModPlatform::ModLoaderType::DataPack &&
        ModPlatform::hasSingleModLoaderSelected(args.loaders.value())) {
        int mappedModLoader = getMappedModLoader(static_cast<ModPlatform::ModLoaderType>(static_cast<int>(args.loaders.value())));
        url += QString("&modLoaderType=%1").arg(mappedModLoader);
    }
    return url;
}
