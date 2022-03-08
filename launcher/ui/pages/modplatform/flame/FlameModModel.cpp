#include "FlameModModel.h"

#include "modplatform/flame/FlameModIndex.h"

namespace FlameMod {

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
const char* ListModel::sorts[6]{ "Featured", "Popularity", "LastUpdated", "Name", "Author", "TotalDownloads" };

void ListModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadIndexedPack(m, obj);
}

void ListModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    FlameMod::loadIndexedPackVersions(m, arr, APPLICATION->network(), m_parent->m_instance);
}

auto ListModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return obj.array();
}

}  // namespace FlameMod
