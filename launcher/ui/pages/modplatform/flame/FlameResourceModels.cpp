#include "FlameResourceModels.h"

#include "Json.h"

#include "modplatform/flame/FlameModIndex.h"

namespace ResourceDownload {

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
const char* FlameModModel::sorts[6]{ "Featured", "Popularity", "LastUpdated", "Name", "Author", "TotalDownloads" };

FlameModModel::FlameModModel(FlameModPage* parent) : ModModel(parent, new FlameAPI) {}

void FlameModModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadIndexedPack(m, obj);
}

// We already deal with the URLs when initializing the pack, due to the API response's structure
void FlameModModel::loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadBody(m, obj);
}

void FlameModModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    FlameMod::loadIndexedPackVersions(m, arr, APPLICATION->network(), &m_associated_page->m_base_instance);
}

auto FlameModModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return Json::ensureArray(obj.object(), "data");
}

}  // namespace ResourceDownload
