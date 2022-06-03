// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
#include "ModrinthAPI.h"

#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/NetJob.h"

static ModrinthAPI api;
static ModPlatform::ProviderCapabilities ProviderCaps;

void Modrinth::loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    pack.addonId = Json::requireString(obj, "project_id");
    pack.provider = ModPlatform::Provider::MODRINTH;
    pack.name = Json::requireString(obj, "title");
    
    QString slug = Json::ensureString(obj, "slug", "");
    if (!slug.isEmpty())
        pack.websiteUrl = "https://modrinth.com/mod/" + Json::ensureString(obj, "slug", "");
    else
        pack.websiteUrl = "";

    pack.description = Json::ensureString(obj, "description", "");

    pack.logoUrl = Json::requireString(obj, "icon_url");
    pack.logoName = pack.addonId.toString();

    ModPlatform::ModpackAuthor modAuthor;
    modAuthor.name = Json::requireString(obj, "author");
    modAuthor.url = api.getAuthorURL(modAuthor.name);
    pack.authors.append(modAuthor);

    // Modrinth can have more data than what's provided by the basic search :)
    pack.extraDataLoaded = false;
}

void Modrinth::loadExtraPackData(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    pack.extraData.issuesUrl = Json::ensureString(obj, "issues_url");
    if(pack.extraData.issuesUrl.endsWith('/'))
        pack.extraData.issuesUrl.chop(1);

    pack.extraData.sourceUrl = Json::ensureString(obj, "source_url");
    if(pack.extraData.sourceUrl.endsWith('/'))
        pack.extraData.sourceUrl.chop(1);

    pack.extraData.wikiUrl = Json::ensureString(obj, "wiki_url");
    if(pack.extraData.wikiUrl.endsWith('/'))
        pack.extraData.wikiUrl.chop(1);

    pack.extraData.discordUrl = Json::ensureString(obj, "discord_url");
    if(pack.extraData.discordUrl.endsWith('/'))
        pack.extraData.discordUrl.chop(1);

    auto donate_arr = Json::ensureArray(obj, "donation_urls");
    for(auto d : donate_arr){
        auto d_obj = Json::requireObject(d);

        ModPlatform::DonationData donate;

        donate.id = Json::ensureString(d_obj, "id");
        donate.platform = Json::ensureString(d_obj, "platform");
        donate.url = Json::ensureString(d_obj, "url");

        pack.extraData.donate.append(donate);
    }

    pack.extraDataLoaded = true;
}

void Modrinth::loadIndexedPackVersions(ModPlatform::IndexedPack& pack,
                                       QJsonArray& arr,
                                       const shared_qobject_ptr<QNetworkAccessManager>& network,
                                       BaseInstance* inst)
{
    QVector<ModPlatform::IndexedVersion> unsortedVersions;
    QString mcVersion = (static_cast<MinecraftInstance*>(inst))->getPackProfile()->getComponentVersion("net.minecraft");

    for (auto versionIter : arr) {
        auto obj = versionIter.toObject();
        auto file = loadIndexedPackVersion(obj);

        if(file.fileId.isValid()) // Heuristic to check if the returned value is valid
            unsortedVersions.append(file);
    }
    auto orderSortPredicate = [](const ModPlatform::IndexedVersion& a, const ModPlatform::IndexedVersion& b) -> bool {
        // dates are in RFC 3339 format
        return a.date > b.date;
    };
    std::sort(unsortedVersions.begin(), unsortedVersions.end(), orderSortPredicate);
    pack.versions = unsortedVersions;
    pack.versionsLoaded = true;
}

auto Modrinth::loadIndexedPackVersion(QJsonObject &obj, QString preferred_hash_type, QString preferred_file_name) -> ModPlatform::IndexedVersion
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
        file.mcVersion.append(mcVer.toString());
    }
    auto loaders = Json::requireArray(obj, "loaders");
    for (auto loader : loaders) {
        file.loaders.append(loader.toString());
    }
    file.version = Json::requireString(obj, "name");
    file.changelog = Json::requireString(obj, "changelog");

    auto files = Json::requireArray(obj, "files");
    int i = 0;

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
        file.is_preferred = Json::requireBoolean(parent, "primary");
        auto hash_list = Json::requireObject(parent, "hashes");
        
        if (hash_list.contains(preferred_hash_type)) {
            file.hash = Json::requireString(hash_list, preferred_hash_type);
            file.hash_type = preferred_hash_type;
        } else {
            auto hash_types = ProviderCaps.hashType(ModPlatform::Provider::MODRINTH);
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
