#include "FlameModIndex.h"

#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/NetJob.h"

void FlameMod::loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    pack.addonId = Json::requireInteger(obj, "id");
    pack.name = Json::requireString(obj, "name");
    pack.websiteUrl = Json::ensureString(obj, "websiteUrl", "");
    pack.description = Json::ensureString(obj, "summary", "");

    bool thumbnailFound = false;
    auto attachments = Json::requireArray(obj, "attachments");
    for (auto attachmentRaw : attachments) {
        auto attachmentObj = Json::requireObject(attachmentRaw);
        bool isDefault = attachmentObj.value("isDefault").toBool(false);
        if (isDefault) {
            thumbnailFound = true;
            pack.logoName = Json::requireString(attachmentObj, "title");
            pack.logoUrl = Json::requireString(attachmentObj, "thumbnailUrl");
            break;
        }
    }

    if (!thumbnailFound) { throw JSONValidationError(QString("Pack without an icon, skipping: %1").arg(pack.name)); }

    auto authors = Json::requireArray(obj, "authors");
    for (auto authorIter : authors) {
        auto author = Json::requireObject(authorIter);
        ModPlatform::ModpackAuthor packAuthor;
        packAuthor.name = Json::requireString(author, "name");
        packAuthor.url = Json::requireString(author, "url");
        pack.authors.append(packAuthor);
    }
}

void FlameMod::loadIndexedPackVersions(ModPlatform::IndexedPack& pack,
                                       QJsonArray& arr,
                                       const shared_qobject_ptr<QNetworkAccessManager>& network,
                                       BaseInstance* inst)
{
    QVector<ModPlatform::IndexedVersion> unsortedVersions;
    bool hasFabric = !(dynamic_cast<MinecraftInstance*>(inst))->getPackProfile()->getComponentVersion("net.fabricmc.fabric-loader").isEmpty();
    QString mcVersion = (dynamic_cast<MinecraftInstance*>(inst))->getPackProfile()->getComponentVersion("net.minecraft");

    for (auto versionIter : arr) {
        auto obj = versionIter.toObject();

        auto versionArray = Json::requireArray(obj, "gameVersion");
        if (versionArray.isEmpty()) { continue; }

        ModPlatform::IndexedVersion file;
        for (auto mcVer : versionArray) {
            file.mcVersion.append(mcVer.toString());
        }

        file.addonId = pack.addonId;
        file.fileId = Json::requireInteger(obj, "id");
        file.date = Json::requireString(obj, "fileDate");
        file.version = Json::requireString(obj, "displayName");
        file.downloadUrl = Json::requireString(obj, "downloadUrl");
        file.fileName = Json::requireString(obj, "fileName");

        auto modules = Json::requireArray(obj, "modules");
        bool is_valid_fabric_version = false;
        for (auto m : modules) {
            auto fname = Json::requireString(m.toObject(), "foldername");
            // FIXME: This does not work properly when a mod supports more than one mod loader, since
            // FIXME: This also doesn't deal with Quilt mods at the moment
            // they bundle the meta files for all of them in the same arquive, even when that version
            // doesn't support the given mod loader.
            if (hasFabric) {
                if (fname == "fabric.mod.json") {
                    is_valid_fabric_version = true;
                    break;
                }
            } else
                break;
            // NOTE: Since we're not validating forge versions, we can just skip this loop.
        }

        if (hasFabric && !is_valid_fabric_version) continue;

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
