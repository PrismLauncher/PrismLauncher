#include "FlamePackIndex.h"

#include "Json.h"

void Flame::loadIndexedPack(Flame::IndexedPack& pack, QJsonObject& obj)
{
    pack.addonId = Json::requireInteger(obj, "id");
    pack.name = Json::requireString(obj, "name");
    pack.description = Json::ensureString(obj, "summary", "");

    auto logo = Json::requireObject(obj, "logo");
    pack.logoName = Json::requireString(logo, "title");
    pack.logoUrl = Json::requireString(logo, "thumbnailUrl");

    auto authors = Json::requireArray(obj, "authors");
    for (auto authorIter : authors) {
        auto author = Json::requireObject(authorIter);
        Flame::ModpackAuthor packAuthor;
        packAuthor.name = Json::requireString(author, "name");
        packAuthor.url = Json::requireString(author, "url");
        pack.authors.append(packAuthor);
    }
    int defaultFileId = Json::requireInteger(obj, "mainFileId");

    bool found = false;
    // check if there are some files before adding the pack
    auto files = Json::requireArray(obj, "latestFiles");
    for (auto fileIter : files) {
        auto file = Json::requireObject(fileIter);
        int id = Json::requireInteger(file, "id");

        // NOTE: for now, ignore everything that's not the default...
        if (id != defaultFileId) {
            continue;
        }

        auto versionArray = Json::requireArray(file, "gameVersions");
        if (versionArray.size() < 1) {
            continue;
        }

        found = true;
        break;
    }
    if (!found) {
        throw JSONValidationError(QString("Pack with no good file, skipping: %1").arg(pack.name));
    }

    loadIndexedInfo(pack, obj);
}

void Flame::loadIndexedInfo(IndexedPack& pack, QJsonObject& obj)
{
    auto links_obj = Json::ensureObject(obj, "links");

    pack.extra.websiteUrl = Json::ensureString(links_obj, "issuesUrl");
    if(pack.extra.websiteUrl.endsWith('/'))
        pack.extra.websiteUrl.chop(1);

    pack.extra.issuesUrl = Json::ensureString(links_obj, "issuesUrl");
    if(pack.extra.issuesUrl.endsWith('/'))
        pack.extra.issuesUrl.chop(1);

    pack.extra.sourceUrl = Json::ensureString(links_obj, "sourceUrl");
    if(pack.extra.sourceUrl.endsWith('/'))
        pack.extra.sourceUrl.chop(1);

    pack.extra.wikiUrl = Json::ensureString(links_obj, "wikiUrl");
    if(pack.extra.wikiUrl.endsWith('/'))
        pack.extra.wikiUrl.chop(1);

    pack.extraInfoLoaded = true;

}

void Flame::loadIndexedPackVersions(Flame::IndexedPack& pack, QJsonArray& arr)
{
    QVector<Flame::IndexedVersion> unsortedVersions;
    for (auto versionIter : arr) {
        auto version = Json::requireObject(versionIter);
        Flame::IndexedVersion file;

        file.addonId = pack.addonId;
        file.fileId = Json::requireInteger(version, "id");
        auto versionArray = Json::requireArray(version, "gameVersions");
        if (versionArray.size() < 1) {
            continue;
        }

        // pick the latest version supported
        file.mcVersion = versionArray[0].toString();
        file.version = Json::requireString(version, "displayName");
        file.downloadUrl = Json::ensureString(version, "downloadUrl");

        // only add if we have a download URL (third party distribution is enabled)
        if (!file.downloadUrl.isEmpty()) {
            unsortedVersions.append(file);
        }
    }

    auto orderSortPredicate = [](const IndexedVersion& a, const IndexedVersion& b) -> bool { return a.fileId > b.fileId; };
    std::sort(unsortedVersions.begin(), unsortedVersions.end(), orderSortPredicate);
    pack.versions = unsortedVersions;
    pack.versionsLoaded = true;
}
