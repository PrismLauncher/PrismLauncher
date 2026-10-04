//
// Created by timoreo on 16/01/2022.
//

#pragma once

#include "Result.h"
#include "modplatform/ModIndex.h"

namespace FlameMod {

Result<> loadIndexedPackVersions(ModPlatform::IndexedPack& pack, const QJsonArray& arr);
Result<ModPlatform::IndexedVersion> loadIndexedPackVersion(const QJsonObject& obj, bool loadChangelog = false);
}  // namespace FlameMod
