#include "ModrinthModel.h"
#include "ModrinthPage.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include <Json.h>

namespace Modrinth {

ListModel::ListModel(ModrinthPage* parent) : ModPlatform::ListModel(parent) {}

ListModel::~ListModel() {}

const char* sorts[5]{ "relevance", "downloads", "follows", "updated", "newest" };

void ListModel::performPaginatedSearch()
{
    QString mcVersion = ((MinecraftInstance*)((ModrinthPage*)parent())->m_instance)->getPackProfile()->getComponentVersion("net.minecraft");
    bool hasFabric = !((MinecraftInstance*)((ModrinthPage*)parent())->m_instance)
                          ->getPackProfile()
                          ->getComponentVersion("net.fabricmc.fabric-loader")
                          .isEmpty();
    auto netJob = new NetJob("Modrinth::Search", APPLICATION->network());
    auto searchUrl = QString(
        "https://api.modrinth.com/v2/search?"
        "offset=%1&"
        "limit=25&"
        "query=%2&"
        "index=%3&"
        "facets=[[\"categories:%4\"],[\"versions:%5\"],[\"project_type:mod\"]]")
            .arg(nextSearchOffset)
            .arg(currentSearchTerm)
            .arg(sorts[currentSort])
            .arg(hasFabric ? "fabric" : "forge")
            .arg(mcVersion);

    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &Modrinth::ListModel::searchRequestFinished);
    QObject::connect(netJob, &NetJob::failed, this, &ListModel::searchRequestFailed);
}

void Modrinth::ListModel::searchRequestFinished()
{
    jobPtr.reset();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from Modrinth at " << parse_error.offset
                   << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }

    QList<ModPlatform::IndexedPack> newList;
    auto packs = doc.object().value("hits").toArray();
    for (auto packRaw : packs) {
        auto packObj = packRaw.toObject();

        ModPlatform::IndexedPack pack;
        try {
            Modrinth::loadIndexedPack(pack, packObj);
            newList.append(pack);
        } catch (const JSONValidationError& e) {
            qWarning() << "Error while loading mod from Modrinth: " << e.cause();
            continue;
        }
    }
    if (packs.size() < 25) {
        searchState = Finished;
    } else {
        nextSearchOffset += 25;
        searchState = CanPossiblyFetchMore;
    }
    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

}  // namespace Modrinth
