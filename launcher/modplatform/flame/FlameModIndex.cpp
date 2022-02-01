#include <QObject>
#include "FlameModIndex.h"
#include "Json.h"
#include "net/NetJob.h"
#include "BaseInstance.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"


void FlameMod::loadIndexedPack(FlameMod::IndexedPack & pack, QJsonObject & obj)
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
        FlameMod::ModpackAuthor packAuthor;
        packAuthor.name = Json::requireString(author, "name");
        packAuthor.url = Json::requireString(author, "url");
        pack.authors.append(packAuthor);
    }
}

void FlameMod::loadIndexedPackVersions(FlameMod::IndexedPack & pack, QJsonArray & arr, const shared_qobject_ptr<QNetworkAccessManager>& network, BaseInstance * inst)
{
    QVector<FlameMod::IndexedVersion> unsortedVersions;
    bool hasFabric = !((MinecraftInstance *)inst)->getPackProfile()->getComponentVersion("net.fabricmc.fabric-loader").isEmpty();
    QString mcVersion = ((MinecraftInstance *)inst)->getPackProfile()->getComponentVersion("net.minecraft");

    for(auto versionIter: arr) {
        auto obj = versionIter.toObject();
        FlameMod::IndexedVersion file;
        file.addonId = pack.addonId;
        file.fileId = Json::requireInteger(obj, "id");
        file.date = Json::requireString(obj, "fileDate");
        auto versionArray = Json::requireArray(obj, "gameVersion");
        if (versionArray.empty()) {
            continue;
        }
        for(auto mcVer : versionArray){
            file.mcVersion.append(mcVer.toString());
        }

        file.version = Json::requireString(obj, "displayName");
        file.downloadUrl = Json::requireString(obj, "downloadUrl");
        file.fileName = Json::requireString(obj, "fileName");

        auto modules = Json::requireArray(obj, "modules");
        bool valid = false;
        for(auto m : modules){
            auto fname = Json::requireString(m.toObject(),"foldername");
            if(hasFabric){
                if(fname == "fabric.mod.json"){
                    valid = true;
                    break;
                }
            }else{
                //this cannot check for the recent mcmod.toml formats
                if(fname == "mcmod.info"){
                    valid = true;
                    break;
                }
            }
        }
        if(!valid && !hasFabric){
            continue;
        }

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