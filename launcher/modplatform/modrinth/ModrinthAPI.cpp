// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModrinthAPI.h"
#include <array>

#include "FileSystem.h"
#include "Json.h"
#include "net/NetRequest.h"
#include "net/RPCSink.h"

namespace {

// Shared parse function for single version responses (currentVersion, latestVersion)
auto makeVersionParseFunc(const QString& taskName) -> Net::RPC::Sink<ModPlatform::IndexedVersion>::ParseFunc
{
    return [taskName](const QByteArray& response) -> Net::RPC::Sink<ModPlatform::IndexedVersion>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from" << taskName << "at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }

        try {
            auto obj = Json::requireObject(doc);
            auto version = Modrinth::loadIndexedPackVersion(obj);
            return version;
        } catch (Json::JsonException& e) {
            qWarning() << "Error while reading Modrinth version info:" << e.cause();
            qDebug() << doc;
            return std::unexpected(e.cause());
        }
    };
}

// Shared parse function for hash-keyed version responses (currentVersions, latestVersions)
auto makeVersionHashParseFunc(const QString& hashFormat, const QString& taskName)
    -> Net::RPC::Sink<QHash<QString, ModPlatform::IndexedVersion>>::ParseFunc
{
    return [hashFormat, taskName](const QByteArray& response) -> Net::RPC::Sink<QHash<QString, ModPlatform::IndexedVersion>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from" << taskName << "at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }

        QHash<QString, ModPlatform::IndexedVersion> versions;
        try {
            auto entries = Json::requireObject(doc);
            for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
                try {
                    auto entry = Json::requireObject(it.value());

                    auto version = Modrinth::loadIndexedPackVersion(entry);
                    auto files = Json::requireArray(entry, "files");
                    for (auto fileVal : files) {
                        auto fileObj = fileVal.toObject();
                        auto hashList = Json::requireObject(fileObj, "hashes");
                        if (hashList.contains(hashFormat)) {
                            auto fileHash = Json::requireString(hashList, hashFormat);
                            if (fileHash == it.key()) {
                                version.downloadUrl = Json::requireString(fileObj, "url");
                                version.fileName = Json::requireString(fileObj, "filename");
                                version.fileName = FS::RemoveInvalidPathChars(version.fileName);
                                version.hash = fileHash;
                                version.hashType = hashFormat;
                                version.isPreferred = true;
                                if (hashList.contains("sha1")) {
                                    version.sha1 = Json::requireString(hashList, "sha1");
                                }
                                if (fileObj.contains("size")) {
                                    version.size = fileObj["size"].toInt();
                                }
                                break;
                            }
                        }
                    }

                    versions.insert(it.key(), version);
                } catch (Json::JsonException& e) {
                    qDebug() << "Skipping invalid version entry for hash" << it.key() << ":" << e.cause();
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
            return std::unexpected(e.cause());
        }
        return versions;
    };
}

}  // namespace

Net::RPC::Spec<ModPlatform::IndexedVersion> ModrinthAPI::currentVersion(const QString& hash, const QString& hashFormat)
{
    auto parseFunc = makeVersionParseFunc("Modrinth::GetCurrentVersion");

    return Net::RPC::Spec<ModPlatform::IndexedVersion>{
        .url = QUrl(QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1?algorithm=%2").arg(hash, hashFormat)),
        .parse = parseFunc,
        .name = "Modrinth::GetCurrentVersion"
    };
}

Net::RPC::Spec<QHash<QString, ModPlatform::IndexedVersion>> ModrinthAPI::currentVersions(const QStringList& hashes, const QString& hashFormat)
{
    QJsonObject bodyObj;
    Json::writeStringList(bodyObj, "hashes", hashes);
    Json::writeString(bodyObj, "algorithm", hashFormat);

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();

auto parseFunc = makeVersionHashParseFunc(hashFormat, "Modrinth::GetCurrentVersions");

    return Net::RPC::Spec<QHash<QString, ModPlatform::IndexedVersion>>{ .method = Net::Request::HttpMethod::Post,
                                                                   .url = QUrl(BuildConfig.MODRINTH_PROD_URL + "/version_files"),
                                                                   .data = bodyRaw,
                                                                   .parse = parseFunc,
                                                                   .name = "Modrinth::GetCurrentVersions" };
}

Net::RPC::Spec<ModPlatform::IndexedVersion> ModrinthAPI::latestVersion(const QString& hash,
                                                                  const QString& hashFormat,
                                                                  std::optional<std::vector<Version>> mcVersions,
                                                                  std::optional<ModPlatform::ModLoaderTypes> loaders)
{
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

    auto parseFunc = makeVersionParseFunc("Modrinth::GetLatestVersion");

    return Net::RPC::Spec<ModPlatform::IndexedVersion>{
        .method = Net::Request::HttpMethod::Post,
        .url = QUrl(QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1/update?algorithm=%2").arg(hash, hashFormat)),
        .data = bodyRaw,
        .parse = parseFunc,
        .name = "Modrinth::GetLatestVersion"
    };
}

Net::RPC::Spec<QHash<QString, ModPlatform::IndexedVersion>> ModrinthAPI::latestVersions(const QStringList& hashes,
                                                                                   const QString& hashFormat,
                                                                                   std::optional<std::vector<Version>> mcVersions,
                                                                                   std::optional<ModPlatform::ModLoaderTypes> loaders)
{
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

    auto parseFunc = makeVersionHashParseFunc(hashFormat, "Modrinth::GetLatestVersions");

    return Net::RPC::Spec<QHash<QString, ModPlatform::IndexedVersion>>{ .method = Net::Request::HttpMethod::Post,
                                                                   .url = QUrl(BuildConfig.MODRINTH_PROD_URL + "/version_files/update"),
                                                                   .data = bodyRaw,
                                                                   .parse = parseFunc,
                                                                   .name = "Modrinth::GetLatestVersions" };
}

Net::RPC::Spec<QList<ModPlatform::IndexedPack::Ptr>> ModrinthAPI::getProjects(QStringList addonIds) const
{
    auto searchUrl = getMultipleModInfoURL(addonIds);

    auto parseFunc = [this](const QByteArray& response) -> Net::RPC::Sink<QList<ModPlatform::IndexedPack::Ptr>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth projects task at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }

        QList<ModPlatform::IndexedPack::Ptr> projects;
        try {
            auto entries = Json::requireArray(doc);
            for (auto entry : entries) {
                auto pack = std::make_shared<ModPlatform::IndexedPack>();
                auto entryObj = Json::requireObject(entry);
                loadIndexedPack(*pack, entryObj);
                projects.append(pack);
            }
        } catch (Json::JsonException& e) {
            qDebug() << doc;
            qWarning() << "Error while reading" << debugName() << "resource info:" << e.cause();
            return std::unexpected(e.cause());
        }
        return projects;
    };

    return Net::RPC::Spec<QList<ModPlatform::IndexedPack::Ptr>>{ .url = QUrl(searchUrl), .parse = parseFunc, .name = "Modrinth::GetProjects" };
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

Net::RPC::Spec<QList<ModPlatform::Category>> ModrinthAPI::getCategories(ModPlatform::ResourceType type) const
{
    auto parseFunc = [type](const QByteArray& response) -> Net::RPC::Sink<QList<ModPlatform::Category>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from categories at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << response;
            return std::unexpected(parseError.errorString());
        }
        const auto resourceType = resourceTypeParameter(type);
        QList<ModPlatform::Category> categories;
        try {
            auto arr = Json::requireArray(doc);

            for (auto val : arr) {
                auto cat = Json::requireObject(val);
                auto name = Json::requireString(cat, "name");
                if (cat["project_type"].toString() == resourceType) {
                    categories.push_back({ .name = name, .id = name });
                }
            }

        } catch (Json::JsonException& e) {
            qCritical() << "Failed to parse response from a version request.";
            qCritical() << e.what();
            qDebug() << doc;
            return std::unexpected(e.what());
        }
        return categories;
    };

    return Net::RPC::Spec<QList<ModPlatform::Category>>{ .url = QUrl(BuildConfig.MODRINTH_PROD_URL + "/tag/category"),
                                                    .parse = parseFunc,
                                                    .name = "ModrinthAPI::getCategories" };
}
