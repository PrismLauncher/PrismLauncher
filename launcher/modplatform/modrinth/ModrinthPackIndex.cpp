#include <QObject>
#include "ModrinthPackIndex.h"

#include "Json.h"
#include "net/NetJob.h"

void Modrinth::loadIndexedPack(Modrinth::IndexedPack & pack, QJsonObject & obj)
{
    pack.addonId = Json::requireString(obj, "mod_id");
    pack.name = Json::requireString(obj, "title");
    pack.websiteUrl = Json::ensureString(obj, "page_url", "");
    pack.description = Json::ensureString(obj, "description", "");

    pack.logoUrl = Json::requireString(obj, "icon_url");
    pack.logoName = "logoName";

    Modrinth::ModpackAuthor packAuthor;
    packAuthor.name = Json::requireString(obj, "author");
    packAuthor.url = Json::requireString(obj, "author_url");
    pack.authors.append(packAuthor); //TODO delete this ? only one author ever exists
}

void Modrinth::loadIndexedPackVersions(Modrinth::IndexedPack & pack, QJsonArray & arr, const shared_qobject_ptr<QNetworkAccessManager>& network)
{
    QVector<Modrinth::IndexedVersion> unsortedVersions;
    for(auto versionIter: arr) {
        auto obj = versionIter.toObject();
        Modrinth::IndexedVersion file;
        file.addonId = Json::requireString(obj,"mod_id") ;
        file.fileId = Json::requireString(obj, "id");
        file.date = Json::requireString(obj, "date_published");
        auto versionArray = Json::requireArray(obj, "game_versions");
        if (versionArray.empty()) {
            continue;
        }
        // pick the latest version supported
        file.mcVersion = versionArray[0].toString();
        file.version = Json::requireString(obj, "name");
        //TODO show all the files ?
        auto parent = Json::requireArray(obj, "files")[0].toObject();
        file.downloadUrl = Json::requireString(parent, "url");
        file.fileName = Json::requireString(parent, "filename");
        unsortedVersions.append(file);
    }
    auto orderSortPredicate = [](const IndexedVersion & a, const IndexedVersion & b) -> bool
    {
        //dates are in RFC 3339 format
        return a.date > b.date;
    };
    std::sort(unsortedVersions.begin(), unsortedVersions.end(), orderSortPredicate);
    pack.versions = unsortedVersions;
    pack.versionsLoaded = true;
}
