#include "FlameModIndex.h"

#include "FileSystem.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"

static FlameAPI api;

void FlameMod::loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    pack.addonId = Json::requireInteger(obj, "id");
    pack.provider = ModPlatform::ResourceProvider::FLAME;
    pack.name = Json::requireString(obj, "name");
    pack.slug = Json::requireString(obj, "slug");
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
            ModPlatform::ModpackAuthor packAuthor;
            packAuthor.name = Json::requireString(author, "name");
            packAuthor.url = Json::requireString(author, "url");
            pack.authors.append(packAuthor);
        }
    }

    pack.extraDataLoaded = false;
    loadURLs(pack, obj);
}

void FlameMod::loadURLs(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    auto links_obj = obj["links"].toObject();

    pack.extraData.issuesUrl = links_obj["issuesUrl"].toString();
    if (pack.extraData.issuesUrl.endsWith('/'))
        pack.extraData.issuesUrl.chop(1);

    pack.extraData.sourceUrl = links_obj["sourceUrl"].toString();
    if (pack.extraData.sourceUrl.endsWith('/'))
        pack.extraData.sourceUrl.chop(1);

    pack.extraData.wikiUrl = links_obj["wikiUrl"].toString();
    if (pack.extraData.wikiUrl.endsWith('/'))
        pack.extraData.wikiUrl.chop(1);

    if (!pack.extraData.body.isEmpty())
        pack.extraDataLoaded = true;
}

void FlameMod::loadBody(ModPlatform::IndexedPack& pack)
{
    pack.extraData.body = api.getModDescription(pack.addonId.toInt());

    if (!pack.extraData.issuesUrl.isEmpty() || !pack.extraData.sourceUrl.isEmpty() || !pack.extraData.wikiUrl.isEmpty())
        pack.extraDataLoaded = true;
}

static QString enumToString(int hash_algorithm)
{
    switch (hash_algorithm) {
        default:
        case 1:
            return "sha1";
        case 2:
            return "md5";
    }
}

void FlameMod::loadIndexedPackVersions(ModPlatform::IndexedPack& pack, QJsonArray& arr)
{
    QList<ModPlatform::IndexedVersion> unsortedVersions;
    for (auto versionIter : arr) {
        auto obj = versionIter.toObject();

        auto file = loadIndexedPackVersion(obj);
        if (!file.addonId.isValid())
            file.addonId = pack.addonId;

        if (file.fileId.isValid())  // Heuristic to check if the returned value is valid
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

auto FlameMod::loadIndexedPackVersion(QJsonObject& obj, bool load_changelog) -> ModPlatform::IndexedVersion
{
    auto versionArray = Json::requireArray(obj, "gameVersions");

    ModPlatform::IndexedVersion file;
    for (auto mcVer : versionArray) {
        auto str = mcVer.toString();

        if (str.contains('.'))
            file.mcVersion.append(str);

        file.side = ModPlatform::Side::NoSide;
        if (auto loader = str.toLower(); loader == "neoforge")
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
        else if (loader == "server" || loader == "client") {
            if (file.side == ModPlatform::Side::NoSide)
                file.side = ModPlatform::SideUtils::fromString(loader);
            else if (file.side != ModPlatform::SideUtils::fromString(loader))
                file.side = ModPlatform::Side::UniversalSide;
        }
    }

    file.addonId = Json::requireInteger(obj, "modId");
    file.fileId = Json::requireInteger(obj, "id");
    file.date = Json::requireString(obj, "fileDate");
    file.version = Json::requireString(obj, "displayName");
    file.downloadUrl = obj["downloadUrl"].toString();
    file.fileName = Json::requireString(obj, "fileName");
    file.fileName = FS::RemoveInvalidPathChars(file.fileName);

    ModPlatform::IndexedVersionType ver_type;
    switch (Json::requireInteger(obj, "releaseType")) {
        case 1:
            ver_type = ModPlatform::IndexedVersionType::Release;
            break;
        case 2:
            ver_type = ModPlatform::IndexedVersionType::Beta;
            break;
        case 3:
            ver_type = ModPlatform::IndexedVersionType::Alpha;
            break;
        default:
            ver_type = ModPlatform::IndexedVersionType::Unknown;
            break;
    }
    file.version_type = ver_type;

    auto hash_list = obj["hashes"].toArray();
    for (auto h : hash_list) {
        auto hash_entry = h.toObject();
        auto hash_types = ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::FLAME);
        auto hash_algo = enumToString(hash_entry["algo"].toInt(1));
        if (hash_types.contains(hash_algo)) {
            file.hash = Json::requireString(hash_entry, "value");
            file.hash_type = hash_algo;
            break;
        }
    }

    auto dependencies = obj["dependencies"].toArray();
    for (auto d : dependencies) {
        auto dep = d.toObject();
        ModPlatform::Dependency dependency;
        dependency.addonId = Json::requireInteger(dep, "modId");
        switch (Json::requireInteger(dep, "relationType")) {
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

    if (load_changelog)
        file.changelog = api.getModFileChangelog(file.addonId.toInt(), file.fileId.toInt());

    return file;
}
