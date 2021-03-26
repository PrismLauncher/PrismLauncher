#include "FlamePackIndex.h"

#include "Json.h"

void Flame::loadIndexedPack(Flame::IndexedPack & pack, QJsonObject & obj)
{
    pack.addonId = Json::requireInteger(obj, "id");
    pack.name = Json::requireString(obj, "name");
    pack.websiteUrl = Json::ensureString(obj, "websiteUrl", "");
    pack.description = Json::ensureString(obj, "summary", "");

    bool thumbnailFound = false;
    auto attachments = Json::requireArray(obj, "attachments");
    for(auto attachmentRaw: attachments) {
        auto attachmentObj = Json::requireObject(attachmentRaw);
        bool isDefault = attachmentObj.value("isDefault").toBool(false);
        if(isDefault) {
            thumbnailFound = true;
            pack.logoName = Json::requireString(attachmentObj, "title");
            pack.logoUrl = Json::requireString(attachmentObj, "thumbnailUrl");
            break;
        }
    }

    if(!thumbnailFound) {
        throw JSONValidationError(QString("Pack without an icon, skipping: %1").arg(pack.name));
    }

    auto authors = Json::requireArray(obj, "authors");
    for(auto authorIter: authors) {
        auto author = Json::requireObject(authorIter);
        Flame::ModpackAuthor packAuthor;
        packAuthor.name = Json::requireString(author, "name");
        packAuthor.url = Json::requireString(author, "url");
        pack.authors.append(packAuthor);
    }
    int defaultFileId = Json::requireInteger(obj, "defaultFileId");

    bool found = false;
    // check if there are some files before adding the pack
    auto files = Json::requireArray(obj, "latestFiles");
    for(auto fileIter: files) {
        auto file = Json::requireObject(fileIter);
        int id = Json::requireInteger(file, "id");

        // NOTE: for now, ignore everything that's not the default...
        if(id != defaultFileId) {
            continue;
        }

        auto versionArray = Json::requireArray(file, "gameVersion");
        if(versionArray.size() < 1) {
            continue;
        }

        found = true;
        break;
    }
    if(!found) {
        throw JSONValidationError(QString("Pack with no good file, skipping: %1").arg(pack.name));
    }
}

void Flame::loadIndexedPackVersions(Flame::IndexedPack & pack, QJsonArray & arr)
{
    QVector<Flame::IndexedVersion> unsortedVersions;
    for(auto versionIter: arr) {
        auto version = Json::requireObject(versionIter);
        Flame::IndexedVersion file; 

        file.addonId = pack.addonId;
        file.fileId = Json::requireInteger(version, "id");
        auto versionArray = Json::requireArray(version, "gameVersion");
        if(versionArray.size() < 1) {
            continue;
        }

        // pick the latest version supported
        file.mcVersion = versionArray[0].toString();
        file.version = Json::requireString(version, "displayName");
        file.downloadUrl = Json::requireString(version, "downloadUrl");
        unsortedVersions.append(file);
    }

    auto orderSortPredicate = [](const IndexedVersion & a, const IndexedVersion & b) -> bool
    {
        return a.fileId > b.fileId;
    };
    std::sort(unsortedVersions.begin(), unsortedVersions.end(), orderSortPredicate);
    pack.versions = unsortedVersions;
    pack.versionsLoaded = true;
}
