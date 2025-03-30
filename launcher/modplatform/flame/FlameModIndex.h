//
// Created by timoreo on 16/01/2022.
//

#pragma once

#include "modplatform/ModIndex.h"

#include "BaseInstance.h"

namespace FlameMod {

void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj);
void loadURLs(ModPlatform::IndexedPack& m, QJsonObject& obj);
void loadBody(ModPlatform::IndexedPack& m);
void loadIndexedPackVersions(ModPlatform::IndexedPack& pack, QJsonArray& arr);
Platform::Version loadIndexedPackVersion(QJsonObject& obj, bool load_changelog = false);
}  // namespace FlameMod