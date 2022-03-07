#include "FlameModModel.h"
#include "FlameModPage.h"
#include "minecraft/PackProfile.h"

#include <Json.h>

namespace FlameMod {

ListModel::ListModel(FlameModPage* parent) : ModPlatform::ListModel(parent) {}

ListModel::~ListModel() {}


void FlameMod::ListModel::searchRequestFinished(QJsonDocument& doc)
{
    jobPtr.reset();

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

const char* sorts[6]{ "Featured", "Popularity", "LastUpdated", "Name", "Author", "TotalDownloads" };

const char** FlameMod::ListModel::getSorts() const
{
    return sorts;
}

}  // namespace FlameMod
