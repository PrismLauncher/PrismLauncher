#include <QObject>
#include "ModrinthPackIndex.h"

#include "Json.h"
#include "net/NetJob.h"
#include "BaseInstance.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"


void Modrinth::loadIndexedPack(Modrinth::IndexedPack & pack, QJsonObject & obj)
{
    pack.addonId = Json::requireString(obj, "project_id");
    pack.name = Json::requireString(obj, "title");
    pack.websiteUrl = Json::ensureString(obj, "page_url", "");
    pack.description = Json::ensureString(obj, "description", "");

    pack.logoUrl = Json::requireString(obj, "icon_url");
    pack.logoName = pack.addonId;

    Modrinth::ModpackAuthor modAuthor;
    modAuthor.name = Json::requireString(obj, "author");
    modAuthor.url = "https://modrinth.com/user/"+modAuthor.name;
    pack.author = modAuthor;
}

void Modrinth::loadIndexedPackVersions(Modrinth::IndexedPack & pack, QJsonArray & arr, const shared_qobject_ptr<QNetworkAccessManager>& network, BaseInstance * inst)
{
    QVector<Modrinth::IndexedVersion> unsortedVersions;
    bool hasFabric = !((MinecraftInstance *)inst)->getPackProfile()->getComponentVersion("net.fabricmc.fabric-loader").isEmpty();
    QString mcVersion = ((MinecraftInstance *)inst)->getPackProfile()->getComponentVersion("net.minecraft");

    for(auto versionIter: arr) {
        auto obj = versionIter.toObject();
        Modrinth::IndexedVersion file;
        file.addonId = Json::requireString(obj,"project_id") ;
        file.fileId = Json::requireString(obj, "id");
        file.date = Json::requireString(obj, "date_published");
        auto versionArray = Json::requireArray(obj, "game_versions");
        if (versionArray.empty()) {
            continue;
        }
        for(auto mcVer : versionArray){
            file.mcVersion.append(mcVer.toString());
        }
        auto loaders = Json::requireArray(obj,"loaders");
        for(auto loader : loaders){
            file.loaders.append(loader.toString());
        }
        file.version = Json::requireString(obj, "name");

        auto files = Json::requireArray(obj, "files");
        int i = 0;
        while (files.count() > 1 && i < files.count()){
            //try to resolve the correct file
            auto parent = files[i].toObject();
            auto fileName = Json::requireString(parent, "filename");
            //avoid grabbing "dev" files
            if(fileName.contains("javadocs",Qt::CaseInsensitive) || fileName.contains("sources",Qt::CaseInsensitive)){
                i++;
                continue;
            }
            //grab the correct mod loader
            if(fileName.contains("forge",Qt::CaseInsensitive) || fileName.contains("fabric",Qt::CaseInsensitive)  ){
                if(hasFabric){
                    if(fileName.contains("forge",Qt::CaseInsensitive)){
                        i++;
                        continue;
                    }
                }else{
                    if(fileName.contains("fabric",Qt::CaseInsensitive)){
                        i++;
                        continue;
                    }
                }
            }
            break;
        }
        auto parent = files[i].toObject();
        if(parent.contains("url")) {
            file.downloadUrl = Json::requireString(parent, "url");
            file.fileName = Json::requireString(parent, "filename");

            unsortedVersions.append(file);
        }
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
