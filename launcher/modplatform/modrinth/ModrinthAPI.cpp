// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModrinthAPI.h"

#include "Application.h"
#include "Json.h"
#include "net/ApiDownload.h"
#include "net/ApiUpload.h"
#include "net/NetJob.h"
#include "net/Upload.h"

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::currentVersion(QString hash, QString hash_format)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCurrentVersion"), APPLICATION->network());

    auto [action, response] =
        Net::ApiDownload::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1?algorithm=%2").arg(hash, hash_format));
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::currentVersions(const QStringList& hashes, QString hash_format)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCurrentVersions"), APPLICATION->network());

    QJsonObject body_obj;

    Json::writeStringList(body_obj, "hashes", hashes);
    Json::writeString(body_obj, "algorithm", hash_format);

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    auto [action, response] = Net::ApiUpload::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files"), body_raw);
    netJob->addNetAction(action);
    netJob->setAskRetry(false);
    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::latestVersion(QString hash,
                                                             QString hash_format,
                                                             std::optional<std::vector<Version>> mcVersions,
                                                             std::optional<ModPlatform::ModLoaderTypes> loaders)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetLatestVersion"), APPLICATION->network());

    QJsonObject body_obj;

    if (loaders.has_value())
        Json::writeStringList(body_obj, "loaders", getModLoaderStrings(loaders.value()));

    if (mcVersions.has_value()) {
        QStringList game_versions;
        for (auto& ver : mcVersions.value()) {
            game_versions.append(mapMCVersionToModrinth(ver));
        }
        Json::writeStringList(body_obj, "game_versions", game_versions);
    }

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    auto [action, response] = Net::ApiUpload::makeByteArray(
        QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1/update?algorithm=%2").arg(hash, hash_format), body_raw);
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::latestVersions(const QStringList& hashes,
                                                              QString hash_format,
                                                              std::optional<std::vector<Version>> mcVersions,
                                                              std::optional<ModPlatform::ModLoaderTypes> loaders)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetLatestVersions"), APPLICATION->network());

    QJsonObject body_obj;

    Json::writeStringList(body_obj, "hashes", hashes);
    Json::writeString(body_obj, "algorithm", hash_format);

    if (loaders.has_value())
        Json::writeStringList(body_obj, "loaders", getModLoaderStrings(loaders.value()));

    if (mcVersions.has_value()) {
        QStringList game_versions;
        for (auto& ver : mcVersions.value()) {
            game_versions.append(mapMCVersionToModrinth(ver));
        }
        Json::writeStringList(body_obj, "game_versions", game_versions);
    }

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();
    auto [action, response] = Net::ApiUpload::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files/update"), body_raw);
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::getProjects(QStringList addonIds) const
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetProjects"), APPLICATION->network());
    auto searchUrl = getMultipleModInfoURL(addonIds);

    auto [action, response] = Net::ApiDownload::makeByteArray(QUrl(searchUrl));
    netJob->addNetAction(action);

    return { netJob, response };
}

QList<ResourceAPI::SortingMethod> ModrinthAPI::getSortingMethods() const
{
    // https://docs.modrinth.com/api-spec/#tag/projects/operation/searchProjects
    return { { 1, "relevance", QObject::tr("Sort by Relevance") },
             { 2, "downloads", QObject::tr("Sort by Downloads") },
             { 3, "follows", QObject::tr("Sort by Follows") },
             { 4, "newest", QObject::tr("Sort by Newest") },
             { 5, "updated", QObject::tr("Sort by Last Updated") } };
}

std::pair<Task::Ptr, QByteArray*> ModrinthAPI::getModCategories()
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCategories"), APPLICATION->network());
    auto [action, response] = Net::ApiDownload::makeByteArray(QUrl(BuildConfig.MODRINTH_PROD_URL + "/tag/category"));
    netJob->addNetAction(action);
    QObject::connect(netJob.get(), &Task::failed, [](QString msg) { qDebug() << "Modrinth failed to get categories:" << msg; });

    return { netJob, response };
}

QList<ModPlatform::Category> ModrinthAPI::loadCategories(const QByteArray& response, QString projectType)
{
    QList<ModPlatform::Category> categories;
    QJsonParseError parse_error{};
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from categories at" << parse_error.offset
                   << "reason:" << parse_error.errorString();
        qWarning() << *response;
        return categories;
    }

    try {
        auto arr = Json::requireArray(doc);

        for (auto val : arr) {
            auto cat = Json::requireObject(val);
            auto name = Json::requireString(cat, "name");
            if (cat["project_type"].toString() == projectType)
                categories.push_back({ name, name });
        }

    } catch (Json::JsonException& e) {
        qCritical() << "Failed to parse response from a version request.";
        qCritical() << e.what();
        qDebug() << doc;
    }
    return categories;
}

QList<ModPlatform::Category> ModrinthAPI::loadModCategories(const QByteArray& response)
{
    return loadCategories(response, "mod");
};
