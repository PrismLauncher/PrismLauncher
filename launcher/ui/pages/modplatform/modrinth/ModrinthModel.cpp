#include "ModrinthModel.h"

#include "modplatform/modrinth/ModrinthPackIndex.h"

namespace Modrinth {

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
const char* ListModel::sorts[5]{ "relevance", "downloads", "follows", "updated", "newest" };

void ListModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    Modrinth::loadIndexedPack(m, obj);
}

void ListModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    Modrinth::loadIndexedPackVersions(m, arr, APPLICATION->network(), m_parent->m_instance);
}

auto ListModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return obj.object().value("hits").toArray();
}

}  // namespace Modrinth
