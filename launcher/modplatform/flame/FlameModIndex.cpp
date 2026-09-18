#include "FlameModIndex.h"

#include <algorithm>

#include "FileSystem.h"
#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"

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

Result<> FlameMod::loadIndexedPackVersions(ModPlatform::IndexedPack& pack, const QJsonArray& arr)
{
    QList<ModPlatform::IndexedVersion> unsortedVersions;
    for (auto versionIter : arr) {
        auto obj = versionIter.toObject();

        TRY_INTO(auto file, loadIndexedPackVersion(obj))
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

Result<ModPlatform::IndexedVersion> FlameMod::loadIndexedPackVersion(const QJsonObject& obj, bool loadChangelog)
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

    if (loadChangelog) {
        file.changelog = FlameAPI::getModFileChangelog(file.addonId.toInt(), file.fileId.toInt());
    }

    return file;
}
