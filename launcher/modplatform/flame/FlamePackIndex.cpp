// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Trial97 <alexandru.tripon97@gmail.com>
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

#include "FlamePackIndex.h"

#include "FileSystem.h"
#include "Json.h"
#include "Result.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceType.h"

namespace {
const auto g_classIDMappings = std::array{
    std::pair{ ModPlatform::ResourceType::Mod, 6 },        std::pair{ ModPlatform::ResourceType::ResourcePack, 12 },
    std::pair{ ModPlatform::ResourceType::World, 17 },     std::pair{ ModPlatform::ResourceType::ShaderPack, 6552 },
    std::pair{ ModPlatform::ResourceType::Modpack, 4471 }, std::pair{ ModPlatform::ResourceType::DataPack, 6945 },
};

QString enumToString(int hashAlgorithm)
{
    switch (hashAlgorithm) {
        default:
        case 1:
            return "sha1";
        case 2:
            return "md5";
    }
}

ModPlatform::ResourceType getResourceType(int classId)
{
    for (auto&& [type, c] : g_classIDMappings) {
        if (c == classId) {
            return type;
        }
    }
    return ModPlatform::ResourceType::Unknown;
}

}  // namespace

namespace Flame::Parse {
Result<> loadIndexedPack(ModPlatform::IndexedPack& pack, const QJsonObject& obj)
{
    TRY_INTO(pack.addonId, Json::requireInteger(obj, "id"))
    pack.provider = ModPlatform::ResourceProvider::FLAME;
    TRY_INTO(pack.name, Json::requireString(obj, "name"))
    TRY_INTO(pack.slug, Json::requireString(obj, "slug"))
    pack.websiteUrl = obj["links"].toObject()["websiteUrl"].toString("");
    pack.description = obj["summary"].toString("");

    QJsonObject logo = obj["logo"].toObject();
    pack.logoName = logo["title"].toString();
    pack.logoUrl = logo["thumbnailUrl"].toString();
    if (pack.logoUrl.isEmpty()) {
        pack.logoUrl = logo["url"].toString();
    }

    auto authors = obj["authors"].toArray();
    if (!authors.isEmpty()) {
        pack.authors.clear();
        for (auto authorIter : authors) {
            TRY_INTO(const auto& author, Json::requireObject(authorIter))
            ModPlatform::ModpackAuthor packAuthor;
            TRY_INTO(packAuthor.name, Json::requireString(author, "name"))
            TRY_INTO(packAuthor.url, Json::requireString(author, "url"))
            pack.authors.append(packAuthor);
        }
    }

    pack.resourceType = getResourceType(obj["classId"].toInt(0));
    pack.extraDataLoaded = false;

    auto linksObj = obj["links"].toObject();

    pack.extraData.issuesUrl = linksObj["issuesUrl"].toString();
    if (pack.extraData.issuesUrl.endsWith('/')) {
        pack.extraData.issuesUrl.chop(1);
    }

    pack.extraData.sourceUrl = linksObj["sourceUrl"].toString();
    if (pack.extraData.sourceUrl.endsWith('/')) {
        pack.extraData.sourceUrl.chop(1);
    }

    pack.extraData.wikiUrl = linksObj["wikiUrl"].toString();
    if (pack.extraData.wikiUrl.endsWith('/')) {
        pack.extraData.wikiUrl.chop(1);
    }

    if (!pack.extraData.body.isEmpty()) {
        pack.extraDataLoaded = true;
    }
    return {};
}

Result<QList<ModPlatform::IndexedVersion>> loadIndexedPackVersions(const QJsonArray& arr,
                                                                   const QString& addonId,
                                                                   ModPlatform::ResourceType resourceType)
{
    QList<ModPlatform::IndexedVersion> unsortedVersions;
    for (auto versionIter : arr) {
        auto obj = versionIter.toObject();

        TRY_INTO(auto file, loadIndexedPackVersion(obj))
        if (resourceType == ModPlatform::ResourceType::TexturePack) {
            // FIXME: Client-side version filtering. This won't take into account any user-selected filtering.
            const auto& mcVersions = file.mcVersion;

            if (!std::any_of(mcVersions.constBegin(), mcVersions.constEnd(),
                             [](const auto& mcVersion) { return Version(mcVersion) <= Version("1.6"); })) {
                continue;
            }
        }
        if (!file.addonId.isValid()) {
            file.addonId = addonId;
        }

        if (file.fileId.isValid()) {  // Heuristic to check if the returned value is valid
            unsortedVersions.append(file);
        }
    }

    auto orderSortPredicate = [](const ModPlatform::IndexedVersion& a, const ModPlatform::IndexedVersion& b) -> bool {
        // dates are in RFC 3339 format
        return a.date > b.date;
    };
    std::ranges::sort(unsortedVersions, orderSortPredicate);
    return unsortedVersions;
}

Result<ModPlatform::IndexedVersion> loadIndexedPackVersion(const QJsonObject& obj)
{
    TRY_INTO(const auto& versionArray, Json::requireArray(obj, "gameVersions"))

    ModPlatform::IndexedVersion file;
    for (auto mcVer : versionArray) {
        auto str = mcVer.toString();

        if (str.contains('.')) {
            file.mcVersion.append(str);
        }

        file.side = ModPlatform::SideType::NoSide;
        if (auto loader = str.toLower(); loader == "neoforge") {
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
        } else if (loader == "server" || loader == "client") {
            if (!file.side.isValid()) {
                file.side = ModPlatform::SideType::fromString(loader);
            } else if (file.side != ModPlatform::SideType::fromString(loader)) {
                file.side = ModPlatform::SideType::UniversalSide;
            }
        }
    }

    TRY_INTO(file.addonId, Json::requireInteger(obj, "modId"))
    TRY_INTO(file.fileId, Json::requireInteger(obj, "id"))
    TRY_INTO(file.date, Json::requireString(obj, "fileDate"))
    TRY_INTO(file.version, Json::requireString(obj, "displayName"))
    file.downloadUrl = obj["downloadUrl"].toString();
    TRY_INTO(file.fileName, Json::requireString(obj, "fileName"))
    file.fileName = FS::RemoveInvalidPathChars(file.fileName);

    TRY_INTO(const auto& releaseType, Json::requireInteger(obj, "releaseType"))
    ModPlatform::IndexedVersionType verType;
    switch (releaseType) {
        case 1:
            verType = ModPlatform::IndexedVersionType::Release;
            break;
        case 2:
            verType = ModPlatform::IndexedVersionType::Beta;
            break;
        case 3:
            verType = ModPlatform::IndexedVersionType::Alpha;
            break;
        default:
            verType = ModPlatform::IndexedVersionType::Unknown;
            break;
    }
    file.versionType = verType;

    auto hashList = obj["hashes"].toArray();
    for (auto h : hashList) {
        auto hashEntry = h.toObject();
        auto hashTypes = ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::FLAME);
        auto hashAlgo = enumToString(hashEntry["algo"].toInt(1));
        if (hashTypes.contains(hashAlgo)) {
            TRY_INTO(file.hash, Json::requireString(hashEntry, "value"))
            file.hashType = hashAlgo;
            break;
        }
    }

    auto dependencies = obj["dependencies"].toArray();
    for (auto d : dependencies) {
        auto dep = d.toObject();
        ModPlatform::Dependency dependency;
        TRY_INTO(dependency.addonId, Json::requireInteger(dep, "modId"))
        TRY_INTO(const auto& relationType, Json::requireInteger(dep, "relationType"))
        switch (relationType) {
            case 1:  // EmbeddedLibrary
                dependency.type = ModPlatform::DependencyType::EMBEDDED;
                break;
            case 2:  // OptionalDependency
                dependency.type = ModPlatform::DependencyType::OPTIONAL;
                break;
            case 3:  // RequiredDependency
                dependency.type = ModPlatform::DependencyType::REQUIRED;
                break;
            case 4:  // Tool
                dependency.type = ModPlatform::DependencyType::TOOL;
                break;
            case 5:  // Incompatible
                dependency.type = ModPlatform::DependencyType::INCOMPATIBLE;
                break;
            case 6:  // Include
                dependency.type = ModPlatform::DependencyType::INCLUDE;
                break;
            default:
                dependency.type = ModPlatform::DependencyType::UNKNOWN;
                break;
        }
        file.dependencies.append(dependency);
    }

    return file;
}

Result<QList<ModPlatform::IndexedPack>> parseProjectList(const QByteArray& response)
{
    QList<ModPlatform::IndexedPack> newList;
    TRY_INTO(auto doc, Json::requireDocument(response, "ResourceAPI")
                           .and_then([](const auto& v) { return Json::requireObject(v); })
                           .and_then([](const auto& v) { return Json::requireArray(v, "data"); }))

    for (auto packRaw : doc) {
        auto packObj = packRaw.toObject();

        ModPlatform::IndexedPack pack;
        TRY(Flame::Parse::loadIndexedPack(pack, packObj))
        newList << pack;
    }
    return newList;
}

int getClassId(ModPlatform::ResourceType type)
{
    for (auto&& [e, classId] : g_classIDMappings) {
        if (e == type) {
            return classId;
        }
    }
    return 0;
}

}  // namespace Flame::Parse
