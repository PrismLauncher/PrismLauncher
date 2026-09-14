// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ModrinthPackIndex.h"
#include "FileSystem.h"
#include "ModrinthAPI.h"

#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceType.h"

namespace {
bool shouldDownloadOnSide(const QString& side)
{
    return side == "required" || side == "optional";
}
}  // namespace

// https://docs.modrinth.com/api/operations/getproject/
Result<> Modrinth::loadIndexedPack(ModPlatform::IndexedPack& pack, const QJsonObject& obj)
{
    pack.addonId = obj["project_id"].toString();
    if (pack.addonId.toString().isEmpty()) {
        TRY_INTO(pack.addonId, Json::requireString(obj, "id"))
    }

    pack.provider = ModPlatform::ResourceProvider::MODRINTH;
    TRY_INTO(pack.name, Json::requireString(obj, "title"))
    pack.resourceType = ModrinthAPI::getResourceType(obj["project_type"].toString());
    if ((obj.contains("loaders") && obj.value("loaders").toArray({}).contains("datapack")) ||
        (obj.contains("all_project_types") && obj.value("all_project_types").toArray({}).contains("datapack"))) {
        pack.resourceType = ModPlatform::ResourceType::DataPack;
    }

    pack.slug = obj["slug"].toString("");
    if (!pack.slug.isEmpty()) {
        pack.websiteUrl = "https://modrinth.com/mod/" + pack.slug;
    } else {
        pack.websiteUrl = "";
    }

    pack.description = obj["description"].toString("");

    pack.logoUrl = obj["icon_url"].toString("");
    pack.logoName = QString("%1.%2").arg(obj["slug"].toString(), QFileInfo(QUrl(pack.logoUrl).fileName()).suffix());

    if (obj.contains("author")) {
        ModPlatform::ModpackAuthor modAuthor;
        modAuthor.name = obj["author"].toString();
        modAuthor.url = ModrinthAPI::getAuthorURL(modAuthor.name);
        pack.authors = { modAuthor };
    }

    auto client = shouldDownloadOnSide(obj["client_side"].toString());
    auto server = shouldDownloadOnSide(obj["server_side"].toString());

    if (server && client) {
        pack.side = ModPlatform::SideType::UniversalSide;
    } else if (server) {
        pack.side = ModPlatform::SideType::ServerSide;
    } else if (client) {
        pack.side = ModPlatform::SideType::ClientSide;
    }

    // Modrinth can have more data than what's provided by the basic search :)
    pack.extraDataLoaded = false;
    return {};
}

Result<> Modrinth::loadExtraPackData(ModPlatform::IndexedPack& pack, const QJsonObject& obj)
{
    pack.extraData.issuesUrl = obj["issues_url"].toString();
    if (pack.extraData.issuesUrl.endsWith('/')) {
        pack.extraData.issuesUrl.chop(1);
    }

    pack.extraData.sourceUrl = obj["source_url"].toString();
    if (pack.extraData.sourceUrl.endsWith('/')) {
        pack.extraData.sourceUrl.chop(1);
    }

    pack.extraData.wikiUrl = obj["wiki_url"].toString();
    if (pack.extraData.wikiUrl.endsWith('/')) {
        pack.extraData.wikiUrl.chop(1);
    }

    pack.extraData.discordUrl = obj["discord_url"].toString();
    if (pack.extraData.discordUrl.endsWith('/')) {
        pack.extraData.discordUrl.chop(1);
    }

    auto donateArr = obj["donation_urls"].toArray();
    for (auto d : donateArr) {
        auto dObj = Json::requireObject(d);
        TRY(dObj)

        ModPlatform::DonationData donate;

        donate.id = dObj.value()["id"].toString();
        donate.platform = dObj.value()["platform"].toString();
        donate.url = dObj.value()["url"].toString();

        pack.extraData.donate.append(donate);
    }

    pack.extraData.status = obj["status"].toString();

    pack.extraData.body = obj["body"].toString().remove("<br>");

    pack.extraDataLoaded = true;
    return {};
}

Result<ModPlatform::IndexedVersion> Modrinth::loadIndexedPackVersion(const QJsonObject& obj,
                                                                     const QString& preferredHashType,
                                                                     const QString& preferredFileName)
{
    ModPlatform::IndexedVersion file;

    TRY_INTO(file.addonId, Json::requireString(obj, "project_id"))
    TRY_INTO(file.fileId, Json::requireString(obj, "id"))
    TRY_INTO(file.date, Json::requireString(obj, "date_published"))
    auto versionArray = Json::requireArray(obj, "game_versions");
    TRY(versionArray)
    if (versionArray->empty()) {
        return {};
    }
    for (auto mcVer : versionArray.value()) {
        file.mcVersion.append({ ModrinthAPI::mapMCVersionFromModrinth(mcVer.toString()),
                                mcVer.toString() });  // double this so we can check both strings when filtering
    }
    auto loaders = Json::requireArray(obj, "loaders");
    TRY(loaders)
    for (auto loader : loaders.value()) {
        if (loader == "neoforge") {
            file.loaders |= ModPlatform::NeoForge;
        } else if (loader == "forge") {
            file.loaders |= ModPlatform::Forge;
        } else if (loader == "cauldron") {
            file.loaders |= ModPlatform::Cauldron;
        } else if (loader == "liteloader") {
            file.loaders |= ModPlatform::LiteLoader;
        } else if (loader == "fabric") {
            file.loaders |= ModPlatform::Fabric;
        } else if (loader == "quilt") {
            file.loaders |= ModPlatform::Quilt;
        }
    }
    TRY_INTO(file.version, Json::requireString(obj, "name"))
    TRY_INTO(file.versionNumber, Json::requireString(obj, "version_number"))
    QString versionType;
    TRY_INTO(versionType, Json::requireString(obj, "version_type"))
    file.versionType = ModPlatform::IndexedVersionType::fromString(versionType);

    if (obj.contains("changelog")) {
        TRY_INTO(file.changelog, Json::requireString(obj, "changelog"))
    }

    auto dependencies = obj["dependencies"].toArray();
    for (auto d : dependencies) {
        auto dep = d.toObject();
        ModPlatform::Dependency dependency;
        dependency.addonId = dep["project_id"].toString();
        dependency.version = dep["version_id"].toString();
        QString depType;
        TRY_INTO(depType, Json::requireString(dep, "dependency_type"))

        if (depType == "required") {
            dependency.type = ModPlatform::DependencyType::REQUIRED;
        } else if (depType == "optional") {
            dependency.type = ModPlatform::DependencyType::OPTIONAL;
        } else if (depType == "incompatible") {
            dependency.type = ModPlatform::DependencyType::INCOMPATIBLE;
        } else if (depType == "embedded") {
            dependency.type = ModPlatform::DependencyType::EMBEDDED;
        } else {
            dependency.type = ModPlatform::DependencyType::UNKNOWN;
        }

        file.dependencies.append(dependency);
    }

    auto files = Json::requireArray(obj, "files");
    TRY(files)
    int i = 0;

    if (files->empty()) {
        // This should not happen normally, but check just in case
        qWarning() << "Modrinth returned an unexpected empty list of files:" << obj;
        return {};
    }

    // Find correct file (needed in cases where one version may have multiple files)
    // Will default to the last one if there's no primary (though I think Modrinth requires that
    // at least one file is primary, idk)
    // NOTE: files.count() is 1-indexed, so we need to subtract 1 to become 0-indexed
    while (i < files->count() - 1) {
        auto parent = files.value()[i].toObject();
        QString fileName;
        TRY_INTO(fileName, Json::requireString(parent, "filename"))

        if (!preferredFileName.isEmpty() && fileName.contains(preferredFileName)) {
            file.isPreferred = true;
            break;
        }

        bool primary = false;
        TRY_INTO(primary, Json::requireBoolean(parent, "primary"))
        // Grab the primary file, if available
        if (primary) {
            break;
        }

        i++;
    }

    auto parent = files.value()[i].toObject();
    if (parent.contains("url")) {
        TRY_INTO(file.downloadUrl, Json::requireString(parent, "url"))
        TRY_INTO(file.fileName, Json::requireString(parent, "filename"))
        file.fileName = FS::RemoveInvalidPathChars(file.fileName);
        bool primary = false;
        TRY_INTO(primary, Json::requireBoolean(parent, "primary"))
        file.isPreferred = primary || (files->count() == 1);
        auto hashList = Json::requireObject(parent, "hashes");
        TRY(hashList)

        if (hashList->contains(preferredHashType)) {
            TRY_INTO(file.hash, Json::requireString(hashList.value(), preferredHashType))
            file.hashType = preferredHashType;
        } else {
            auto hashTypes = ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH);
            for (auto& hashType : hashTypes) {
                if (hashList->contains(hashType)) {
                    TRY_INTO(file.hash, Json::requireString(hashList.value(), hashType))
                    file.hashType = hashType;
                    break;
                }
            }
        }

        return file;
    }

    return {};
}
