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
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/ModIndex.h"

static ModrinthAPI api;

bool shouldDownloadOnSide(QString side)
{
    return side == "required" || side == "optional";
}

// https://docs.modrinth.com/api/operations/getproject/
void Modrinth::loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    pack.addonId = obj["project_id"].toString();
    if (pack.addonId.toString().isEmpty())
        pack.addonId = Json::requireString(obj, "id");

    pack.provider = ModPlatform::ResourceProvider::MODRINTH;
    pack.name = Json::requireString(obj, "title");

    pack.slug = obj["slug"].toString("");
    if (!pack.slug.isEmpty())
        pack.websiteUrl = "https://modrinth.com/mod/" + pack.slug;
    else
        pack.websiteUrl = "";

    pack.description = obj["description"].toString("");

    pack.logoUrl = obj["icon_url"].toString("");
    pack.logoName = QString("%1.%2").arg(obj["slug"].toString(), QFileInfo(QUrl(pack.logoUrl).fileName()).suffix());

    if (obj.contains("author")) {
        ModPlatform::ModpackAuthor modAuthor;
        modAuthor.name = obj["author"].toString();
        modAuthor.url = api.getAuthorURL(modAuthor.name);
        pack.authors = { modAuthor };
    }

    auto client = shouldDownloadOnSide(obj["client_side"].toString());
    auto server = shouldDownloadOnSide(obj["server_side"].toString());

    if (server && client) {
        pack.side = ModPlatform::Side::UniversalSide;
    } else if (server) {
        pack.side = ModPlatform::Side::ServerSide;
    } else if (client) {
        pack.side = ModPlatform::Side::ClientSide;
    }

    // Modrinth can have more data than what's provided by the basic search :)
    pack.extraDataLoaded = false;
}

void Modrinth::loadExtraPackData(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    pack.extraData.issuesUrl = obj["issues_url"].toString();
    if (pack.extraData.issuesUrl.endsWith('/'))
        pack.extraData.issuesUrl.chop(1);

    pack.extraData.sourceUrl = obj["source_url"].toString();
    if (pack.extraData.sourceUrl.endsWith('/'))
        pack.extraData.sourceUrl.chop(1);

    pack.extraData.wikiUrl = obj["wiki_url"].toString();
    if (pack.extraData.wikiUrl.endsWith('/'))
        pack.extraData.wikiUrl.chop(1);

    pack.extraData.discordUrl = obj["discord_url"].toString();
    if (pack.extraData.discordUrl.endsWith('/'))
        pack.extraData.discordUrl.chop(1);

    auto donate_arr = obj["donation_urls"].toArray();
    for (auto d : donate_arr) {
        auto d_obj = Json::requireObject(d);

        ModPlatform::DonationData donate;

        donate.id = d_obj["id"].toString();
        donate.platform = d_obj["platform"].toString();
        donate.url = d_obj["url"].toString();

        pack.extraData.donate.append(donate);
    }

    pack.extraData.status = obj["status"].toString();

    pack.extraData.body = obj["body"].toString().remove("<br>");

    pack.extraDataLoaded = true;
}

ModPlatform::IndexedVersion Modrinth::loadIndexedPackVersion(QJsonObject& obj, QString preferred_hash_type, QString preferred_file_name)
{
    ModPlatform::IndexedVersion file;

    file.addonId = Json::requireString(obj, "project_id");
    file.fileId = Json::requireString(obj, "id");
    file.date = Json::requireString(obj, "date_published");
    auto versionArray = Json::requireArray(obj, "game_versions");
    if (versionArray.empty()) {
        return {};
    }
    for (auto mcVer : versionArray) {
        file.mcVersion.append({ ModrinthAPI::mapMCVersionFromModrinth(mcVer.toString()),
                                mcVer.toString() });  // double this so we can check both strings when filtering
    }
    auto loaders = Json::requireArray(obj, "loaders");
    for (auto loader : loaders) {
        if (loader == "neoforge")
            file.loaders |= ModPlatform::NeoForge;
        else if (loader == "forge")
            file.loaders |= ModPlatform::Forge;
        else if (loader == "cauldron")
            file.loaders |= ModPlatform::Cauldron;
        else if (loader == "liteloader")
            file.loaders |= ModPlatform::LiteLoader;
        else if (loader == "fabric")
            file.loaders |= ModPlatform::Fabric;
        else if (loader == "quilt")
            file.loaders |= ModPlatform::Quilt;
    }
    file.version = Json::requireString(obj, "name");
    file.version_number = Json::requireString(obj, "version_number");
    file.version_type = ModPlatform::IndexedVersionType::fromString(Json::requireString(obj, "version_type"));

    file.changelog = Json::requireString(obj, "changelog");

    auto dependencies = obj["dependencies"].toArray();
    for (auto d : dependencies) {
        auto dep = d.toObject();
        ModPlatform::Dependency dependency;
        dependency.addonId = dep["project_id"].toString();
        dependency.version = dep["version_id"].toString();
        auto depType = Json::requireString(dep, "dependency_type");

        if (depType == "required")
            dependency.type = ModPlatform::DependencyType::REQUIRED;
        else if (depType == "optional")
            dependency.type = ModPlatform::DependencyType::OPTIONAL;
        else if (depType == "incompatible")
            dependency.type = ModPlatform::DependencyType::INCOMPATIBLE;
        else if (depType == "embedded")
            dependency.type = ModPlatform::DependencyType::EMBEDDED;
        else
            dependency.type = ModPlatform::DependencyType::UNKNOWN;

        file.dependencies.append(dependency);
    }

    auto files = Json::requireArray(obj, "files");
    int i = 0;

    if (files.empty()) {
        // This should not happen normally, but check just in case
        qWarning() << "Modrinth returned an unexpected empty list of files:" << obj;
        return {};
    }

    // Find correct file (needed in cases where one version may have multiple files)
    // Will default to the last one if there's no primary (though I think Modrinth requires that
    // at least one file is primary, idk)
    // NOTE: files.count() is 1-indexed, so we need to subtract 1 to become 0-indexed
    while (i < files.count() - 1) {
        auto parent = files[i].toObject();
        auto fileName = Json::requireString(parent, "filename");

        if (!preferred_file_name.isEmpty() && fileName.contains(preferred_file_name)) {
            file.is_preferred = true;
            break;
        }

        // Grab the primary file, if available
        if (Json::requireBoolean(parent, "primary"))
            break;

        i++;
    }

    auto parent = files[i].toObject();
    if (parent.contains("url")) {
        file.downloadUrl = Json::requireString(parent, "url");
        file.fileName = Json::requireString(parent, "filename");
        file.fileName = FS::RemoveInvalidPathChars(file.fileName);
        file.is_preferred = Json::requireBoolean(parent, "primary") || (files.count() == 1);
        auto hash_list = Json::requireObject(parent, "hashes");

        if (hash_list.contains(preferred_hash_type)) {
            file.hash = Json::requireString(hash_list, preferred_hash_type);
            file.hash_type = preferred_hash_type;
        } else {
            auto hash_types = ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH);
            for (auto& hash_type : hash_types) {
                if (hash_list.contains(hash_type)) {
                    file.hash = Json::requireString(hash_list, hash_type);
                    file.hash_type = hash_type;
                    break;
                }
            }
        }

        return file;
    }

    return {};
}
