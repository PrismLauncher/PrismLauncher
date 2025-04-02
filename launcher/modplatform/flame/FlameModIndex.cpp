#include "FlameModIndex.h"

#include "FileSystem.h"
#include "Json.h"
#include "api/structures/Project.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/flame/FlameAPI.h"

static FlameAPI api;

Platform::ResourceType getResourceType(int classId)
{
    switch (classId) {
        case 17:  // Worlds
            return Platform::ResourceType::World;
        case 6:  // Mods
            return Platform::ResourceType::Mod;
        case 12:  // Resource Packs
                  // return Platform::ResourceType::ResourcePack; // not really a resourcepack
            /* fallthrough */
        case 4546:  // Customization
                    // return Platform::ResourceType::ShaderPack; // not really a shaderPack
            /* fallthrough */
        case 4471:  // Modpacks
            /* fallthrough */
        case 5:  // Bukkit Plugins
            /* fallthrough */
        case 4559:  // Addons
            /* fallthrough */
        default:
            return Platform::ResourceType::Unknown;
    }
}

void FlameMod::loadIndexedPack(Platform::Project& pack, QJsonObject& obj)
{
    pack.projectId = Json::requireInteger(obj, "id");
    pack.provider = Platform::Provider::FLAME;
    pack.name = Json::requireString(obj, "name");
    pack.slug = Json::requireString(obj, "slug");
    pack.websiteUrl = Json::ensureString(Json::ensureObject(obj, "links"), "websiteUrl", "");
    pack.description = Json::ensureString(obj, "summary", "");

    QJsonObject logo = Json::ensureObject(obj, "logo");
    pack.logoName = Json::ensureString(logo, "title");
    pack.logoUrl = Json::ensureString(logo, "thumbnailUrl");
    if (pack.logoUrl.isEmpty()) {
        pack.logoUrl = Json::ensureString(logo, "url");
    }

    auto authors = Json::ensureArray(obj, "authors");
    for (auto authorIter : authors) {
        auto author = Json::requireObject(authorIter);
        Platform::ModpackAuthor packAuthor;
        packAuthor.name = Json::requireString(author, "name");
        packAuthor.url = Json::requireString(author, "url");
        pack.authors.append(packAuthor);
    }
    if (obj.contains("classId")) {
        pack.resourceType = getResourceType(Json::ensureInteger(obj, "classId", 0, "modClassId"));
    }
    pack.extraDataLoaded = false;
    loadURLs(pack, obj);
}

void FlameMod::loadURLs(Platform::Project& pack, QJsonObject& obj)
{
    auto links_obj = Json::ensureObject(obj, "links");

    pack.extraData.issuesUrl = Json::ensureString(links_obj, "issuesUrl");
    if (pack.extraData.issuesUrl.endsWith('/'))
        pack.extraData.issuesUrl.chop(1);

    pack.extraData.sourceUrl = Json::ensureString(links_obj, "sourceUrl");
    if (pack.extraData.sourceUrl.endsWith('/'))
        pack.extraData.sourceUrl.chop(1);

    pack.extraData.wikiUrl = Json::ensureString(links_obj, "wikiUrl");
    if (pack.extraData.wikiUrl.endsWith('/'))
        pack.extraData.wikiUrl.chop(1);
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

auto FlameMod::loadIndexedPackVersion(QJsonObject& obj, bool load_changelog) -> Platform::Version
{
    auto versionArray = Json::requireArray(obj, "gameVersions");

    Platform::Version file;
    for (auto mcVer : versionArray) {
        auto str = mcVer.toString();

        if (str.contains('.'))
            file.mcVersion.append(str);

        file.side = Platform::Side::NoSide;
        if (auto loader = str.toLower(); loader == "neoforge")
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
        else if (loader == "server" || loader == "client") {
            if (file.side == Platform::Side::NoSide)
                file.side = Platform::SideUtils::fromString(loader);
            else if (file.side != Platform::SideUtils::fromString(loader))
                file.side = Platform::Side::UniversalSide;
        }
    }

    file.projectId = Json::requireInteger(obj, "modId");
    file.fileId = Json::requireInteger(obj, "id");
    file.date = Json::requireString(obj, "fileDate");
    file.version = Json::requireString(obj, "displayName");
    file.downloadUrl = Json::ensureString(obj, "downloadUrl");
    file.fileName = Json::requireString(obj, "fileName");
    file.fileName = FS::RemoveInvalidPathChars(file.fileName);

    Platform::VersionType ver_type;
    switch (Json::requireInteger(obj, "releaseType")) {
        case 1:
            ver_type = Platform::VersionType::Release;
            break;
        case 2:
            ver_type = Platform::VersionType::Beta;
            break;
        case 3:
            ver_type = Platform::VersionType::Alpha;
            break;
        default:
            ver_type = Platform::VersionType::Unknown;
    }
    file.version_type = ver_type;

    auto hash_list = Json::ensureArray(obj, "hashes");
    for (auto h : hash_list) {
        auto hash_entry = Json::ensureObject(h);
        auto hash_types = Platform::ProviderUtils::hashType(Platform::Provider::FLAME);
        auto hash_algo = enumToString(Json::ensureInteger(hash_entry, "algo", 1, "algorithm"));
        if (hash_types.contains(hash_algo)) {
            file.hash = Json::requireString(hash_entry, "value");
            file.hash_type = hash_algo;
            break;
        }
    }

    auto dependencies = Json::ensureArray(obj, "dependencies");
    for (auto d : dependencies) {
        auto dep = Json::ensureObject(d);
        Platform::Dependency dependency;
        dependency.projectId = Json::requireInteger(dep, "modId");
        switch (Json::requireInteger(dep, "relationType")) {
            case 1:  // EmbeddedLibrary
                dependency.type = Platform::DependencyType::EMBEDDED;
                break;
            case 2:  // OptionalDependency
                dependency.type = Platform::DependencyType::OPTIONAL;
                break;
            case 3:  // RequiredDependency
                dependency.type = Platform::DependencyType::REQUIRED;
                break;
            case 4:  // Tool
                dependency.type = Platform::DependencyType::TOOL;
                break;
            case 5:  // Incompatible
                dependency.type = Platform::DependencyType::INCOMPATIBLE;
                break;
            case 6:  // Include
                dependency.type = Platform::DependencyType::INCLUDE;
                break;
            default:
                dependency.type = Platform::DependencyType::UNKNOWN;
                break;
        }
        file.dependencies.append(dependency);
    }

    if (load_changelog)
        file.changelog = api.getModFileChangelog(file.projectId.toInt(), file.fileId.toInt());

    return file;
}
