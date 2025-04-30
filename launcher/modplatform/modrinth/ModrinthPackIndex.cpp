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

#include "Json.h"
#include "api/structures/Hash.h"
#include "api/structures/Project.h"
#include "api/structures/Provider.h"
#include "api/structures/VersionType.h"
#include "modplatform/helpers/HashUtils.h"

bool shouldDownloadOnSide(QString side)
{
    return side == "required" || side == "optional";
}
QString getAuthorURL(const QString& name)
{
    return "https://modrinth.com/user/" + name;
};

// https://docs.modrinth.com/api-spec/#tag/projects/operation/getProject
void Modrinth::loadIndexedPack(Platform::Project& pack, QJsonObject& obj)
{
    pack.projectId = Json::ensureString(obj, "project_id");
    if (pack.projectId.toString().isEmpty())
        pack.projectId = Json::requireString(obj, "id");

    pack.provider = Platform::Provider::MODRINTH;
    pack.name = Json::requireString(obj, "title");

    pack.slug = Json::ensureString(obj, "slug", "");
    if (!pack.slug.isEmpty())
        pack.websiteUrl = "https://modrinth.com/mod/" + pack.slug;
    else
        pack.websiteUrl = "";

    pack.description = Json::ensureString(obj, "description", "");

    pack.logoUrl = Json::ensureString(obj, "icon_url", "");
    pack.logoName = pack.projectId.toString();

    Platform::ModpackAuthor modAuthor;
    modAuthor.name = Json::ensureString(obj, "author", QObject::tr("No author(s)"));
    modAuthor.url = getAuthorURL(modAuthor.name);
    pack.authors.append(modAuthor);

    auto client = shouldDownloadOnSide(Json::ensureString(obj, "client_side"));
    auto server = shouldDownloadOnSide(Json::ensureString(obj, "server_side"));

    if (server && client) {
        pack.side = Platform::Side::UniversalSide;
    } else if (server) {
        pack.side = Platform::Side::ServerSide;
    } else if (client) {
        pack.side = Platform::Side::ClientSide;
    }

    // Modrinth can have more data than what's provided by the basic search :)
    pack.extraDataLoaded = false;
}

void Modrinth::loadExtraPackData(Platform::Project& pack, QJsonObject& obj)
{
    pack.extraData.issuesUrl = Json::ensureString(obj, "issues_url");
    if (pack.extraData.issuesUrl.endsWith('/'))
        pack.extraData.issuesUrl.chop(1);

    pack.extraData.sourceUrl = Json::ensureString(obj, "source_url");
    if (pack.extraData.sourceUrl.endsWith('/'))
        pack.extraData.sourceUrl.chop(1);

    pack.extraData.wikiUrl = Json::ensureString(obj, "wiki_url");
    if (pack.extraData.wikiUrl.endsWith('/'))
        pack.extraData.wikiUrl.chop(1);

    pack.extraData.discordUrl = Json::ensureString(obj, "discord_url");
    if (pack.extraData.discordUrl.endsWith('/'))
        pack.extraData.discordUrl.chop(1);

    auto donate_arr = Json::ensureArray(obj, "donation_urls");
    for (auto d : donate_arr) {
        auto d_obj = Json::requireObject(d);

        Platform::DonationData donate;

        donate.id = Json::ensureString(d_obj, "id");
        donate.platform = Json::ensureString(d_obj, "platform");
        donate.url = Json::ensureString(d_obj, "url");

        pack.extraData.donate.append(donate);
    }

    pack.extraData.status = Json::ensureString(obj, "status");

    pack.extraData.body = Json::ensureString(obj, "body").remove("<br>");

    pack.extraDataLoaded = true;
}

QString mapMCVersionFromModrinth(QString v)
{
    static const QString preString = " Pre-Release ";
    bool pre = false;
    if (v.contains("-pre")) {
        pre = true;
        v.replace("-pre", preString);
    }
    v.replace("-", " ");
    if (pre) {
        v.replace(" Pre Release ", preString);
    }
    return v;
}

Platform::Version Modrinth::loadIndexedPackVersion(QJsonObject& obj, Hashing::Algorithm preferred_hash_type, QString preferred_file_name)
{
    Platform::Version file;

    file.projectId = Json::requireString(obj, "project_id");
    file.fileId = Json::requireString(obj, "id");
    file.date = Json::requireString(obj, "date_published");
    auto versionArray = Json::requireArray(obj, "game_versions");
    if (versionArray.empty()) {
        return {};
    }
    for (auto mcVer : versionArray) {
        file.mcVersion.append(mapMCVersionFromModrinth(mcVer.toString()));
    }
    auto loaders = Json::requireArray(obj, "loaders");
    for (auto loader : loaders) {
        if (loader == "neoforge")
            file.loaders |= Platform::ModLoader::NeoForge;
        else if (loader == "forge")
            file.loaders |= Platform::ModLoader::Forge;
        else if (loader == "cauldron")
            file.loaders |= Platform::ModLoader::Cauldron;
        else if (loader == "liteloader")
            file.loaders |= Platform::ModLoader::LiteLoader;
        else if (loader == "fabric")
            file.loaders |= Platform::ModLoader::Fabric;
        else if (loader == "quilt")
            file.loaders |= Platform::ModLoader::Quilt;
    }
    file.version = Json::requireString(obj, "name");
    file.version_number = Json::requireString(obj, "version_number");
    file.version_type = Platform::VersionTypeUtils::fromString(Json::requireString(obj, "version_type"));

    file.changelog = Json::requireString(obj, "changelog");

    auto dependencies = Json::ensureArray(obj, "dependencies");
    for (auto d : dependencies) {
        auto dep = Json::ensureObject(d);
        Platform::Dependency dependency;
        dependency.projectId = Json::ensureString(dep, "project_id");
        dependency.version = Json::ensureString(dep, "version_id");
        auto depType = Json::requireString(dep, "dependency_type");

        if (depType == "required")
            dependency.type = Platform::DependencyType::REQUIRED;
        else if (depType == "optional")
            dependency.type = Platform::DependencyType::OPTIONAL;
        else if (depType == "incompatible")
            dependency.type = Platform::DependencyType::INCOMPATIBLE;
        else if (depType == "embedded")
            dependency.type = Platform::DependencyType::EMBEDDED;
        else
            dependency.type = Platform::DependencyType::UNKNOWN;

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
        file.size = Json::ensureInteger("size", 0);
        auto hash_list = Json::requireObject(parent, "hashes");
        auto preferred_hash_type_str = Hashing::algorithmToString(preferred_hash_type);
        if (hash_list.contains(preferred_hash_type_str)) {
            file.hashes << Platform::Hash{ preferred_hash_type, Json::requireString(hash_list, preferred_hash_type_str) };
        }
        auto hash_types = Platform::ProviderUtils::hashTypeAlg(Platform::Provider::MODRINTH);
        for (auto& hash_type : hash_types) {
            auto hashStr = Hashing::algorithmToString(hash_type);
            if (hash_type != preferred_hash_type && hash_list.contains(hashStr)) {
                file.hashes << Platform::Hash{ hash_type, Json::requireString(hash_list, hashStr) };
            }
        }

        return file;
    }

    return {};
}
