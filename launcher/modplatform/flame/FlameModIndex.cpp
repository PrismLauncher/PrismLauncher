#include "FlameModIndex.h"

#include <algorithm>

#include "FileSystem.h"
#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"

Result<> FlameMod::loadIndexedPack(ModPlatform::IndexedPack& pack, const QJsonObject& obj)
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
            auto author = Json::requireObject(authorIter);
            TRY(author)
            ModPlatform::ModpackAuthor packAuthor;
            TRY_INTO(packAuthor.name, Json::requireString(*author, "name"))
            TRY_INTO(packAuthor.url, Json::requireString(*author, "url"))
            pack.authors.append(packAuthor);
        }
    }

    pack.resourceType = FlameAPI::getResourceType(obj["classId"].toInt(0));
    pack.extraDataLoaded = false;
    loadURLs(pack, obj);
    return {};
}

void FlameMod::loadURLs(ModPlatform::IndexedPack& pack, const QJsonObject& obj)
{
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
}

void FlameMod::loadBody(ModPlatform::IndexedPack& pack)
{
    pack.extraData.body = FlameAPI::getModDescription(pack.addonId.toInt());

    if (!pack.extraData.issuesUrl.isEmpty() || !pack.extraData.sourceUrl.isEmpty() || !pack.extraData.wikiUrl.isEmpty()) {
        pack.extraDataLoaded = true;
    }
}

namespace {
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
}  // namespace

Result<> FlameMod::loadIndexedPackVersions(ModPlatform::IndexedPack& pack, QJsonArray& arr)
{
    QList<ModPlatform::IndexedVersion> unsortedVersions;
    for (auto versionIter : arr) {
        auto obj = versionIter.toObject();

        auto fileRes = loadIndexedPackVersion(obj);
        TRY(fileRes)
        auto& file = fileRes.value();
        if (!file.addonId.isValid()) {
            file.addonId = pack.addonId;
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
    pack.versions = unsortedVersions;
    pack.versionsLoaded = true;
    return {};
}

Result<ModPlatform::IndexedVersion> FlameMod::loadIndexedPackVersion(QJsonObject& obj, bool loadChangelog)
{
    auto versionArray = Json::requireArray(obj, "gameVersions");
    TRY(versionArray)

    ModPlatform::IndexedVersion file;
    for (auto mcVer : versionArray.value()) {
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

    int releaseType = 0;
    TRY_INTO(releaseType, Json::requireInteger(obj, "releaseType"))
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
        int relationType = 0;
        TRY_INTO(relationType, Json::requireInteger(dep, "relationType"))
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

    if (loadChangelog) {
        file.changelog = FlameAPI::getModFileChangelog(file.addonId.toInt(), file.fileId.toInt());
    }

    return file;
}
