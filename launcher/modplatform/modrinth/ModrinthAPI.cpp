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

QString mapMCVersionToModrinth(Version v)
{
    static const QString preString = " Pre-Release ";
    auto verStr = v.toString();

    if (verStr.contains(preString)) {
        verStr.replace(preString, "-pre");
    }
    verStr.replace(" ", "-");
    return verStr;
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
