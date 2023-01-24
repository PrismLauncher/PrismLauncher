// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "FlameResourceModels.h"

#include "Json.h"

#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"

namespace ResourceDownload {

FlameModModel::FlameModModel(BaseInstance const& base) : ModModel(base, new FlameAPI) {}

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
    FlameMod::loadIndexedPackVersions(m, arr, APPLICATION->network(), &m_base_instance);
}

auto FlameModModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return Json::ensureArray(obj.object(), "data");
}

}  // namespace ResourceDownload
