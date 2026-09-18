// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "FlameAPI.h"
#include <qstringview.h>
#include <optional>
#include "BuildConfig.h"

#include "Application.h"
#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlamePackIndex.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"
#include "net/Request.h"

std::pair<Task::Ptr, QByteArray*> FlameAPI::matchFingerprints(const QList<uint>& fingerprints)
{
    auto netJob = makeShared<NetJob>(QString("Flame::MatchFingerprints"), APPLICATION->network());

    QJsonObject bodyObj;
    QJsonArray fingerprintsArr;
    for (const auto& fp : fingerprints) {
        fingerprintsArr.append(QString("%1").arg(fp));
    }

    bodyObj["fingerprints"] = fingerprintsArr;

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();
    auto [action, response] = Net::ApiRequest::makeByteArray(QString(BuildConfig.FLAME_BASE_URL + "/fingerprints"), bodyRaw);
    netJob->addNetAction(action);

    return { netJob, response };
}

QString FlameAPI::getModFileChangelog(int modId, int fileId)
{
    QEventLoop lock;
    QString changelog;

    auto netJob = makeShared<NetJob>(QString("Flame::FileChangelog"), APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(
        QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files/%2/changelog")
            .arg(QString::fromStdString(std::to_string(modId)), QString::fromStdString(std::to_string(fileId))));
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::succeeded, netJob.get(), [&netJob, response, &changelog] {
        auto doc = Json::requireDocument(*response, "Flame::FileChangelog");
        if (!doc) {
            qWarning() << "Error while parsing JSON response from Flame::FileChangelog:" << doc.error();
            qWarning() << *response;

            netJob->failed(doc.error());
            return;
        }

        changelog = doc->object()["data"].toString();
    });

    QObject::connect(netJob.get(), &NetJob::finished, &lock, &QEventLoop::quit);

    netJob->start();
    lock.exec();

    return changelog;
}

std::pair<Task::Ptr, QByteArray*> FlameAPI::getFiles(const QStringList& fileIds)
{
    auto netJob = makeShared<NetJob>(QString("Flame::GetFiles"), APPLICATION->network());

    QJsonObject bodyObj;
    QJsonArray filesArr;
    for (const auto& fileId : fileIds) {
        filesArr.append(fileId);
    }

    bodyObj["fileIds"] = filesArr;

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();

    auto [action, response] = Net::ApiRequest::makeByteArray(QString(BuildConfig.FLAME_BASE_URL + "/mods/files"), bodyRaw);
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::failed, netJob.get(), [bodyRaw] { qDebug() << bodyRaw; });

    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> FlameAPI::getFile(const QString& addonId, const QString& fileId)
{
    auto netJob = makeShared<NetJob>(QString("Flame::GetFile"), APPLICATION->network());
    auto [action, response] =
        Net::ApiRequest::makeByteArray(QUrl(QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files/%2").arg(addonId, fileId)));
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::failed, netJob.get(),
                     [addonId, fileId] { qDebug() << "Flame API file failure" << addonId << fileId; });

    return { netJob, response };
}

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

namespace {
const auto g_classIDMappings = std::array{
    std::pair{ ModPlatform::ResourceType::Mod, 6 },        std::pair{ ModPlatform::ResourceType::ResourcePack, 12 },
    std::pair{ ModPlatform::ResourceType::World, 17 },     std::pair{ ModPlatform::ResourceType::ShaderPack, 6552 },
    std::pair{ ModPlatform::ResourceType::Modpack, 4471 }, std::pair{ ModPlatform::ResourceType::DataPack, 6945 },
};

int getClassId(ModPlatform::ResourceType type)
{
    for (auto&& [e, classId] : g_classIDMappings) {
        if (e == type) {
            return classId;
        }
    }
    return 0;
}

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

std::optional<QString> FlameAPI::getDependencyURL(const DependencySearchArgs& args) const
{
    auto addonId = args.dependency.addonId.toString();
    auto url = QString(BuildConfig.FLAME_BASE_URL + "/mods/%1/files?pageSize=10000&gameVersion=%2").arg(addonId, args.mcVersion.toString());
    if ((args.loader != 0U) && ModPlatform::hasSingleModLoaderSelected(args.loader)) {
        int mappedModLoader = getMappedModLoader(static_cast<ModPlatform::ModLoaderType>(static_cast<int>(args.loader)));
        url += QString("&modLoaderType=%1").arg(mappedModLoader);
    }
    return url;
}

QUrl FlameAPI::searchProjectsURL(const SearchArgs& args)
{
    QStringList getArguments;
    getArguments.append(QString("classId=%1").arg(getClassId(args.type)));
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

ModPlatform::ResourceType FlameAPI::getResourceType(int classId)
{
    for (auto&& [type, c] : g_classIDMappings) {
        if (c == classId) {
            return type;
        }
    }
    return ModPlatform::ResourceType::Unknown;
}

std::optional<QString> FlameAPI::getVersionsURL(const VersionSearchArgs& args) const
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

std::pair<Task::Ptr, QByteArray*> FlameAPI::getCategories(ModPlatform::ResourceType type)
{
    auto netJob = makeShared<NetJob>(QString("Flame::GetCategories"), APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(
        QUrl(QString(BuildConfig.FLAME_BASE_URL + "/categories?gameId=432&classId=%1").arg(getClassId(type))));
    netJob->addNetAction(action);
    QObject::connect(netJob.get(), &Task::failed, netJob.get(),
                     [](const QString& msg) { qDebug() << "Flame failed to get categories:" << msg; });
    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> FlameAPI::getModCategories() const
{
    return getCategories(ModPlatform::ResourceType::Mod);
}

QList<ModPlatform::Category> FlameAPI::loadModCategories(const QByteArray& response) const
{
    QList<ModPlatform::Category> categories;
    auto parse = [&response, &categories] -> Result<> {
        TRY_INTO(const auto& doc, Json::requireObject(response).and_then([](const auto& v) { return Json::requireArray(v, "data"); }))

        for (auto val : doc) {
            TRY_INTO(const auto& cat, Json::requireObject(val))
            TRY_INTO(const auto& id, Json::requireInteger(cat, "id"))
            TRY_INTO(const auto& name, Json::requireString(cat, "name"))
            categories.push_back({ .name = name, .id = QString::number(id) });
        }
        return {};
    };
    if (auto res = parse(); !res) {
        qCritical() << "Failed to parse response from categories:" << res.error();
        qDebug() << response;
    }
    return categories;
};

std::optional<ModPlatform::IndexedVersion> FlameAPI::getLatestVersion(const QList<ModPlatform::IndexedVersion>& versions,
                                                                      const QList<ModPlatform::ModLoaderType>& instanceLoaders,
                                                                      ModPlatform::ModLoaderTypes fallback,
                                                                      bool checkLoaders)
{
    static const auto s_noLoader = ModPlatform::ModLoaderType(0);
    if (!checkLoaders) {
        std::optional<ModPlatform::IndexedVersion> ver;
        for (const auto& fileTmp : versions) {
            if (!ver.has_value() || fileTmp.date > ver->date) {
                ver = fileTmp;
            }
        }
        return ver;
    }
    QHash<ModPlatform::ModLoaderType, ModPlatform::IndexedVersion> bestMatch;
    auto checkVersion = [&bestMatch](const ModPlatform::IndexedVersion& version, const ModPlatform::ModLoaderType& loader) {
        if (bestMatch.contains(loader)) {
            auto best = bestMatch.value(loader);
            if (version.date > best.date) {
                bestMatch[loader] = version;
            }
        } else {
            bestMatch[loader] = version;
        }
    };
    for (const auto& fileTmp : versions) {
        auto loaders = ModPlatform::modLoaderTypesToList(fileTmp.loaders);
        if (loaders.isEmpty()) {
            checkVersion(fileTmp, s_noLoader);
        } else {
            for (auto loader : loaders) {
                checkVersion(fileTmp, loader);
            }
        }
    }
    // edge case: mod has installed for forge but the instance is fabric => fabric version will be prioritizated on update
    auto currentLoaders = instanceLoaders + ModPlatform::modLoaderTypesToList(fallback);
    currentLoaders.append(s_noLoader);  // add a fallback in case the versions do not define a loader

    for (auto loader : currentLoaders) {
        if (bestMatch.contains(loader)) {
            auto bestForLoader = bestMatch.value(loader);
            // awkward case where the mod has only two loaders and one of them is not specified
            if (loader != s_noLoader && bestMatch.contains(s_noLoader) && bestMatch.size() == 2) {
                auto bestForNoLoader = bestMatch.value(s_noLoader);
                if (bestForNoLoader.date > bestForLoader.date) {
                    return bestForNoLoader;
                }
            }
            return bestForLoader;
        }
    }
    return {};
}
