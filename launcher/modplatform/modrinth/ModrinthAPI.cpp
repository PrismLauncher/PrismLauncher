// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModrinthAPI.h"
#include <array>

#include "Application.h"
#include "Json.h"
#include "modplatform/ResourceType.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"

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

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::getProjects(QStringList addonIds) const
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetProjects"), APPLICATION->network());
    auto searchUrl = getMultipleModInfoURL(addonIds);

    auto [action, response] = Net::ApiRequest::makeByteArray(QUrl(searchUrl));
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
namespace {
const auto g_resourceTypeMap = std::array{
    std::pair{ ModPlatform::ResourceType::Mod, "mod" },           std::pair{ ModPlatform::ResourceType::ResourcePack, "resourcepack" },
    std::pair{ ModPlatform::ResourceType::ShaderPack, "shader" }, std::pair{ ModPlatform::ResourceType::DataPack, "datapack" },
    std::pair{ ModPlatform::ResourceType::Modpack, "modpack" },
};
}

ModPlatform::ResourceType ModrinthAPI::getResourceType(const QString& param)
{
    for (const auto& [key, value] : g_resourceTypeMap) {
        if (value == param) {
            return key;
        }
    }

    qWarning() << "Invalid resource type for Modrinth API!" << param;
    return ModPlatform::ResourceType::Unknown;
}

QString ModrinthAPI::resourceTypeParameter(ModPlatform::ResourceType type)
{
    for (const auto& [key, value] : g_resourceTypeMap) {
        if (key == type) {
            return value;
        }
    }

    qWarning() << "Invalid resource type for Modrinth API!" << static_cast<std::uint8_t>(type);
    return "";
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::getModCategories() const
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCategories"), APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(QUrl(BuildConfig.MODRINTH_PROD_URL + "/tag/category"));
    netJob->addNetAction(action);
    QObject::connect(netJob.get(), &Task::failed, netJob.get(),
                     [](const QString& msg) { qDebug() << "Modrinth failed to get categories:" << msg; });

    return { netJob, response };
}

QList<ModPlatform::Category> ModrinthAPI::loadCategories(const QByteArray& response, const QString& projectType)
{
    QList<ModPlatform::Category> categories;
    QJsonParseError parseError{};
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from categories at" << parseError.offset << "reason:" << parseError.errorString();
        qWarning() << *response;
        return categories;
    }

    try {
        auto arr = Json::requireArray(doc);

        for (auto val : arr) {
            auto cat = Json::requireObject(val);
            auto name = Json::requireString(cat, "name");
            if (cat["project_type"].toString() == projectType) {
                categories.push_back({ .name = name, .id = name });
            }
        }

    } catch (Json::JsonException& e) {
        qCritical() << "Failed to parse response from a version request.";
        qCritical() << e.what();
        qDebug() << doc;
    }
    return categories;
}

QList<ModPlatform::Category> ModrinthAPI::loadModCategories(const QByteArray& response) const
{
    return loadCategories(response, "mod");
}
