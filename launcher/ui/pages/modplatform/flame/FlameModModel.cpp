#include "FlameModModel.h"
#include "FlameModPage.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include <Json.h>

namespace FlameMod {

ListModel::ListModel(FlameModPage* parent) : ModPlatform::ListModel(parent) {}

ListModel::~ListModel() {}

const char* sorts[6]{ "Featured", "Popularity", "LastUpdated", "Name", "Author", "TotalDownloads" };

void ListModel::performPaginatedSearch()
{
    QString mcVersion = ((MinecraftInstance*)((FlameModPage*)parent())->m_instance)->getPackProfile()->getComponentVersion("net.minecraft");
    bool hasFabric = !((MinecraftInstance*)((FlameModPage*)parent())->m_instance)
                          ->getPackProfile()
                          ->getComponentVersion("net.fabricmc.fabric-loader")
                          .isEmpty();
    auto netJob = new NetJob("Flame::Search", APPLICATION->network());
    auto searchUrl = QString(
        "https://addons-ecs.forgesvc.net/api/v2/addon/search?"
        "gameId=432&"
        "categoryId=0&"
        "sectionId=6&"

        "index=%1&"
        "pageSize=25&"
        "searchFilter=%2&"
        "sort=%3&"
        "modLoaderType=%4&"
        "gameVersion=%5")
            .arg(nextSearchOffset)
            .arg(currentSearchTerm)
            .arg(sorts[currentSort])
            .arg(hasFabric ? 4 : 1)  // Enum: https://docs.curseforge.com/?http#tocS_ModLoaderType
            .arg(mcVersion);

    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &FlameMod::ListModel::searchRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void FlameMod::ListModel::searchRequestFinished()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from Flame at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    QList<ModPlatform::IndexedPack> newList;
    auto packs = doc.array();
    for(auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        ModPlatform::IndexedPack pack;
        try
        {
            FlameMod::loadIndexedPack(pack, packObj);
            newList.append(pack);
        }
        catch(const JSONValidationError &e)
        {
            qWarning() << "Error while loading mod from Flame: " << e.cause();
            continue;
        }
    }
    if(packs.size() < 25) {
        searchState = Finished;
    } else {
        nextSearchOffset += 25;
        searchState = CanPossiblyFetchMore;
    }
    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

}  // namespace FlameMod
