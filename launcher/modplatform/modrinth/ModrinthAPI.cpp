// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModrinthAPI.h"

#include "Application.h"
#include "FileSystem.h"
#include "Json.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"
#include "net/NetRequest.h"
#include "net/RPCSink.h"

Net::Spec<ModPlatform::IndexedVersion> ModrinthAPI::currentVersion(const QString& hash, const QString& hashFormat)
{
    auto parseFunc = [](const QByteArray& response) -> Net::RpcSink<ModPlatform::IndexedVersion>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth::GetCurrentVersion at" << parseError.offset
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

    return Net::Spec<ModPlatform::IndexedVersion>{
        .url = QUrl(QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1?algorithm=%2").arg(hash, hashFormat)),
        .parse = parseFunc,
        .name = "Modrinth::GetCurrentVersion"
    };
}

Net::Spec<QHash<QString, ModPlatform::IndexedVersion>> ModrinthAPI::currentVersions(const QStringList& hashes, const QString& hashFormat)
{
    QJsonObject bodyObj;
    Json::writeStringList(bodyObj, "hashes", hashes);
    Json::writeString(bodyObj, "algorithm", hashFormat);

    QJsonDocument body(bodyObj);
    auto bodyRaw = body.toJson();

    auto parseFunc = [hashFormat](const QByteArray& response) -> Net::RpcSink<QHash<QString, ModPlatform::IndexedVersion>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth::GetCurrentVersions at" << parseError.offset
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

                    // First load the base version info
                    auto version = Modrinth::loadIndexedPackVersion(entry);

                    // Now find the specific file that matches the requested hash (it.key())
                    auto files = Json::requireArray(entry, "files");
                    for (auto fileVal : files) {
                        auto fileObj = fileVal.toObject();
                        auto hashList = Json::requireObject(fileObj, "hashes");
                        if (hashList.contains(hashFormat)) {
                            auto fileHash = Json::requireString(hashList, hashFormat);
                            if (fileHash == it.key()) {
                                // Found the matching file, update version with this file's info
                                version.downloadUrl = Json::requireString(fileObj, "url");
                                version.fileName = Json::requireString(fileObj, "filename");
                                version.fileName = FS::RemoveInvalidPathChars(version.fileName);
                                version.hash = fileHash;
                                version.hash_type = hashFormat;
                                version.is_preferred = true;

                                // Also get sha1 and size if available
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
                    // Skip missing/invalid keys as per design
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
            return std::unexpected(e.cause());
        }
        return versions;
    };

    return Net::Spec<QHash<QString, ModPlatform::IndexedVersion>>{ .method = Net::NetRequest::HttpMethod::Post,
                                                                   .url = QUrl(BuildConfig.MODRINTH_PROD_URL + "/version_files"),
                                                                   .data = bodyRaw,
                                                                   .parse = parseFunc,
                                                                   .name = "Modrinth::GetCurrentVersions" };
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

Net::Spec<QList<ModPlatform::IndexedPack::Ptr>> ModrinthAPI::getProjects(QStringList addonIds) const
{
    auto searchUrl = getMultipleModInfoURL(addonIds);

    auto parseFunc = [this](const QByteArray& response) -> Net::RpcSink<QList<ModPlatform::IndexedPack::Ptr>>::ParseResult {
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

    return Net::Spec<QList<ModPlatform::IndexedPack::Ptr>>{ .url = QUrl(searchUrl), .parse = parseFunc, .name = "Modrinth::GetProjects" };
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

Net::Spec<QList<ModPlatform::Category>> ModrinthAPI::getCategories(ModPlatform::ResourceType type) const
{
    auto parseFunc = [type](const QByteArray& response) -> Net::RpcSink<QList<ModPlatform::Category>>::ParseResult {
        QJsonParseError parseError{};
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from categories at" << parseError.offset
                       << "reason:" << parseError.errorString();
            qWarning() << *response;
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

    return Net::Spec<QList<ModPlatform::Category>>{ .url = QUrl(BuildConfig.MODRINTH_PROD_URL + "/tag/category"),
                                                    .parse = parseFunc,
                                                    .name = "ModrinthAPI::getCategories" };
}
