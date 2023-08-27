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

Task::Ptr ModrinthAPI::currentVersion(QString hash, QString hash_format, std::shared_ptr<QByteArray> response)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCurrentVersion"), APPLICATION->network());

    netJob->addNetAction(Net::ApiDownload::makeByteArray(
        QString(BuildConfig.MODRINTH_PROD_URL + "/version_file/%1?algorithm=%2").arg(hash, hash_format), response));

    return netJob;
}

Task::Ptr ModrinthAPI::currentVersions(const QStringList& hashes, QString hash_format, std::shared_ptr<QByteArray> response)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetCurrentVersions"), APPLICATION->network());

    QJsonObject body_obj;

    Json::writeStringList(body_obj, "hashes", hashes);
    Json::writeString(body_obj, "algorithm", hash_format);

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(Net::ApiUpload::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files"), response, body_raw));

    return netJob;
}

Task::Ptr ModrinthAPI::latestVersion(QString hash,
                                     QString hash_format,
                                     std::optional<std::list<Version>> mcVersions,
                                     std::optional<ModPlatform::ModLoaderTypes> loaders,
                                     std::shared_ptr<QByteArray> response)
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetLatestVersion"), APPLICATION->network());

    QJsonObject body_obj;

    if (loaders.has_value())
        Json::writeStringList(body_obj, "loaders", getModLoaderStrings(loaders.value()));

    if (mcVersions.has_value()) {
        QStringList game_versions;
        for (auto& ver : mcVersions.value()) {
            game_versions.append(ver.toString());
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
                                      std::optional<ModPlatform::ModLoaderTypes> loaders,
                                      std::shared_ptr<QByteArray> response)
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
            game_versions.append(ver.toString());
        }
        Json::writeStringList(body_obj, "game_versions", game_versions);
    }

    QJsonDocument body(body_obj);
    auto body_raw = body.toJson();

    netJob->addNetAction(
        Net::ApiUpload::makeByteArray(QString(BuildConfig.MODRINTH_PROD_URL + "/version_files/update"), response, body_raw));

    return netJob;
}

Task::Ptr ModrinthAPI::getProjects(QStringList addonIds, std::shared_ptr<QByteArray> response) const
{
    auto netJob = makeShared<NetJob>(QString("Modrinth::GetProjects"), APPLICATION->network());
    auto searchUrl = getMultipleModInfoURL(addonIds);

    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(searchUrl), response));

    return netJob;
}

// https://docs.modrinth.com/api-spec/#tag/projects/operation/searchProjects
static QList<ResourceAPI::SortingMethod> s_sorts = { { 1, "relevance", QObject::tr("Sort by Relevance") },
                                                     { 2, "downloads", QObject::tr("Sort by Downloads") },
                                                     { 3, "follows", QObject::tr("Sort by Follows") },
                                                     { 4, "newest", QObject::tr("Sort by Last Updated") },
                                                     { 5, "updated", QObject::tr("Sort by Newest") } };

QList<ResourceAPI::SortingMethod> ModrinthAPI::getSortingMethods() const
{
    return s_sorts;
}
