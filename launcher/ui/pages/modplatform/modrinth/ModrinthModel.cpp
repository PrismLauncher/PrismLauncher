#include "ModrinthModel.h"
#include "ModrinthPage.h"
#include "minecraft/MinecraftInstance.h"

#include <Json.h>

namespace Modrinth {

ListModel::ListModel(ModrinthPage* parent) : ModPlatform::ListModel(parent) {}

ListModel::~ListModel() {}

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

const char* sorts[5]{ "relevance", "downloads", "follows", "updated", "newest" };

const char** Modrinth::ListModel::getSorts() const
{
    return sorts;
}

}  // namespace Modrinth
