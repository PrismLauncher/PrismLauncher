//
// Created by timoreo on 16/01/2022.
//

#pragma once

#include "Result.h"
#include "modplatform/ModIndex.h"

namespace FlameMod {

Result<> loadIndexedPack(ModPlatform::IndexedPack& pack, const QJsonObject& obj);
void loadURLs(ModPlatform::IndexedPack& pack, const QJsonObject& obj);
void loadBody(ModPlatform::IndexedPack& pack);
Result<> loadIndexedPackVersions(ModPlatform::IndexedPack& pack, QJsonArray& arr);
Result<ModPlatform::IndexedVersion> loadIndexedPackVersion(QJsonObject& obj, bool loadChangelog = false);
}  // namespace FlameMod
