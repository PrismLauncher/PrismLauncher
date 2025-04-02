// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModrinthAPI.h"

#include "Application.h"
#include "BuildConfig.h"
#include "Json.h"
#include "api/Api.h"
#include "net/ApiDownload.h"
#include "net/ApiUpload.h"
#include "net/NetJob.h"
#include "net/Upload.h"

Task::Ptr ModrinthAPI::currentVersion(QString hash, QString hash_format, std::shared_ptr<QByteArray> response)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCurrentVersion"), APPLICATION->network());

    netJob->addNetAction(Net::ApiDownload::makeByteArray(
        QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1?algorithm=%2").arg(hash, hash_format), response));

    return netJob;
}

NetJob::Ptr ModrinthAPI::currentVersions(const QStringList& hashes, QString hash_format, std::shared_ptr<QByteArray> response)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCurrentVersions"), APPLICATION->network());

    QJsonObject body_obj;

    Json::writeStringList(body_obj, "hashes", hashes);
    Json::writeString(body_obj, "algorithm", hash_format);

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::ApiUpload::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files"), response, body_raw));
    netJob->setAskRetry(false);
    return netJob;
}
QStringList getModLoaderStrings2(const Platform::ModLoaders types)
{
    QStringList l;
    for (auto loader : { Platform::ModLoader::NeoForge, Platform::ModLoader::Forge, Platform::ModLoader::Fabric, Platform::ModLoader::Quilt,
                         Platform::ModLoader::LiteLoader }) {
        if (types & loader) {
            l << Platform::ModloaderUtils::toString(loader);
        }
    }
    return l;
}

Task::Ptr ModrinthAPI::latestVersion(QString hash,
                                     QString hash_format,
                                     std::optional<std::list<Version>> mcVersions,
                                     std::optional<Platform::ModLoaders> loaders,
                                     std::shared_ptr<QByteArray> response)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetLatestVersion"), APPLICATION->network());

    QJsonObject body_obj;

    if (loaders.has_value())
        Json::writeStringList(body_obj, "loaders", getModLoaderStrings2(loaders.value()));

    if (mcVersions.has_value()) {
        QStringList game_versions;
        for (auto& ver : mcVersions.value()) {
            game_versions.append(mapMCVersionToModrinth(ver));
        }
        Json::writeStringList(body_obj, "game_versions", game_versions);
    }

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::ApiUpload::makeByteArray(
        QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1/update?algorithm=%2").arg(hash, hash_format), response, body_raw));

    return netJob;
}

Task::Ptr ModrinthAPI::latestVersions(const QStringList& hashes,
                                      QString hash_format,
                                      std::optional<std::list<Version>> mcVersions,
                                      std::optional<Platform::ModLoaders> loaders,
                                      std::shared_ptr<QByteArray> response)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetLatestVersions"), APPLICATION->network());

    QJsonObject body_obj;

    Json::writeStringList(body_obj, "hashes", hashes);
    Json::writeString(body_obj, "algorithm", hash_format);

    if (loaders.has_value())
        Json::writeStringList(body_obj, "loaders", getModLoaderStrings2(loaders.value()));

    if (mcVersions.has_value()) {
        QStringList game_versions;
        for (auto& ver : mcVersions.value()) {
            game_versions.append(mapMCVersionToModrinth(ver));
        }
        Json::writeStringList(body_obj, "game_versions", game_versions);
    }

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(
        Net::ApiUpload::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files/update"), response, body_raw));

    return netJob;
}

Task::Ptr ModrinthAPI::getModCategories(std::shared_ptr<QByteArray> response)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCategories"), APPLICATION->network());
    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(BuildConfig.MODRINTH_PROD_URL + "/tag/category"), response));
    QObject::connect(netJob.get(), &Task::failed, [](QString msg) { qDebug() << "Modrinth failed to get categories:" << msg; });
    return netJob;
}

QList<Platform::Category> ModrinthAPI::loadCategories(std::shared_ptr<QByteArray> response, QString projectType)
{
    QList<Platform::Category> categories;
    QJsonParseError parse_error{};
    QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from categories at " << parse_error.offset
                   << " reason: " << parse_error.errorString();
        qWarning() << *response;
        return categories;
    }

    try {
        auto arr = Json::requireArray(doc);

        for (auto val : arr) {
            auto cat = Json::requireObject(val);
            auto name = Json::requireString(cat, "name");
            if (Json::ensureString(cat, "project_type", "") == projectType)
                categories.push_back({ name, name });
        }

    } catch (Json::JsonException& e) {
        qCritical() << "Failed to parse response from a version request.";
        qCritical() << e.what();
        qDebug() << doc;
    }
    return categories;
}

QList<Platform::Category> ModrinthAPI::loadModCategories(std::shared_ptr<QByteArray> response)
{
    return loadCategories(response, "mod");
};
