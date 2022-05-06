// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

auto Modrinth::loadIndexedPackVersion(QJsonObject &obj) -> ModPlatform::IndexedVersion
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

    auto files = Json::requireArray(obj, "files");
    int i = 0;

    // Find correct file (needed in cases where one version may have multiple files)
    // Will default to the last one if there's no primary (though I think Modrinth requires that
    // at least one file is primary, idk)
    // NOTE: files.count() is 1-indexed, so we need to subtract 1 to become 0-indexed
    while (i < files.count() - 1) {
        auto parent = files[i].toObject();
        auto fileName = Json::requireString(parent, "filename");

        // Grab the primary file, if available
        if (Json::requireBoolean(parent, "primary"))
            break;

        i++;
    }

    auto parent = files[i].toObject();
    if (parent.contains("url")) {
        file.downloadUrl = Json::requireString(parent, "url");
        file.fileName = Json::requireString(parent, "filename");
        auto hash_list = Json::requireObject(parent, "hashes");
        auto hash_types = ProviderCaps.hashType(ModPlatform::Provider::MODRINTH);
        for (auto& hash_type : hash_types) {
            if (hash_list.contains(hash_type)) {
                file.hash = Json::requireString(hash_list, hash_type);
                file.hash_type = hash_type;
                break;
            }
        }

        return file;
    }

    return {};
}
