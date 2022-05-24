#include "FlameModIndex.h"

#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/flame/FlameAPI.h"
#include "net/NetJob.h"

void FlameMod::loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    pack.addonId = Json::requireInteger(obj, "id");
    pack.name = Json::requireString(obj, "name");
    pack.websiteUrl = Json::ensureString(Json::ensureObject(obj, "links"), "websiteUrl", "");
    pack.description = Json::ensureString(obj, "summary", "");

    QJsonObject logo = Json::requireObject(obj, "logo");
    pack.logoName = Json::requireString(logo, "title");
    pack.logoUrl = Json::requireString(logo, "thumbnailUrl");

    auto authors = Json::requireArray(obj, "authors");
    for (auto authorIter : authors) {
        auto author = Json::requireObject(authorIter);
        ModPlatform::ModpackAuthor packAuthor;
        packAuthor.name = Json::requireString(author, "name");
        packAuthor.url = Json::requireString(author, "url");
        pack.authors.append(packAuthor);
    }

    loadExtraPackData(pack, obj);
}

void FlameMod::loadExtraPackData(ModPlatform::IndexedPack& pack, QJsonObject& obj)
{
    auto links_obj = Json::ensureObject(obj, "links");

    pack.extraData.issuesUrl = Json::ensureString(links_obj, "issuesUrl");
    if(pack.extraData.issuesUrl.endsWith('/'))
        pack.extraData.issuesUrl.chop(1);

    pack.extraData.sourceUrl = Json::ensureString(links_obj, "sourceUrl");
    if(pack.extraData.sourceUrl.endsWith('/'))
        pack.extraData.sourceUrl.chop(1);

    pack.extraData.wikiUrl = Json::ensureString(links_obj, "wikiUrl");
    if(pack.extraData.wikiUrl.endsWith('/'))
        pack.extraData.wikiUrl.chop(1);

    pack.extraDataLoaded = true;
}

void FlameMod::loadIndexedPackVersions(ModPlatform::IndexedPack& pack,
                                       QJsonArray& arr,
                                       const shared_qobject_ptr<QNetworkAccessManager>& network,
                                       BaseInstance* inst)
{
    QVector<ModPlatform::IndexedVersion> unsortedVersions;
    auto profile = (dynamic_cast<MinecraftInstance*>(inst))->getPackProfile();
    QString mcVersion = profile->getComponentVersion("net.minecraft");

    for (auto versionIter : arr) {
        auto obj = versionIter.toObject();

        auto versionArray = Json::requireArray(obj, "gameVersions");
        if (versionArray.isEmpty()) {
            continue;
        }

        ModPlatform::IndexedVersion file;
        for (auto mcVer : versionArray) {
            auto str = mcVer.toString();

            if (str.contains('.'))
                file.mcVersion.append(str);
        }

        file.addonId = pack.addonId;
        file.fileId = Json::requireInteger(obj, "id");
        file.date = Json::requireString(obj, "fileDate");
        file.version = Json::requireString(obj, "displayName");
        file.downloadUrl = Json::requireString(obj, "downloadUrl");
        file.fileName = Json::requireString(obj, "fileName");

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
