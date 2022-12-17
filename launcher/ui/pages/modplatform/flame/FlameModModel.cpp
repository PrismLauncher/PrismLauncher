#include "FlameModModel.h"
#include "Json.h"
#include "modplatform/flame/FlameModIndex.h"

namespace FlameMod {

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
const char* ListModel::sorts[6]{ "Featured", "Popularity", "LastUpdated", "Name", "Author", "TotalDownloads" };

void ListModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadIndexedPack(m, obj);
}

// We already deal with the URLs when initializing the pack, due to the API response's structure
void ListModel::loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    FlameMod::loadBody(m, obj);
}

void ListModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    FlameMod::loadIndexedPackVersions(m, arr, APPLICATION->network(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance);
}

auto ListModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return Json::ensureArray(obj.object(), "data");
}

}  // namespace FlameMod
