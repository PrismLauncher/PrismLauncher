//
// Created by timoreo on 16/01/2022.
//

#pragma once

#include "modplatform/ModIndex.h"

class FlameAPI;

namespace FlameMod {

void loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj);
void loadURLs(ModPlatform::IndexedPack& pack, QJsonObject& obj);
void loadBody(ModPlatform::IndexedPack& pack, const FlameAPI& api);
void loadIndexedPackVersions(ModPlatform::IndexedPack& pack, QJsonArray& arr);
/** Pass changelogApi to also fetch the file changelog (blocking network call). */
ModPlatform::IndexedVersion loadIndexedPackVersion(QJsonObject& obj, const FlameAPI* changelogApi = nullptr);
}  // namespace FlameMod
