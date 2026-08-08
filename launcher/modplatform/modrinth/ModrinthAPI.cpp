// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModrinthAPI.h"

#include "Application.h"
#include "Json.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::currentVersion(const QString& hash, const QString& hashFormat) const
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCurrentVersion"), APPLICATION->network());

    auto [action, response] =
        Net::ApiRequest::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1?algorithm=%2").arg(hash, hashFormat));
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::currentVersions(const QStringList& hashes, QString hashFormat) const
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
