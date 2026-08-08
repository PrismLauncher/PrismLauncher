//
// Created by timoreo on 16/01/2022.
//

#pragma once

#include "modplatform/ModIndex.h"

namespace FlameMod {

struct FingerprintMatch {
    qint64 fileFingerprint;
    qint64 modId;
    qint64 fileId;
    bool isAvailable;
};

ModPlatform::ResourceType getResourceType(int classId);
void loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj);
void loadURLs(ModPlatform::IndexedPack& pack, QJsonObject& obj);
void loadBody(ModPlatform::IndexedPack& pack);
void loadIndexedPackVersions(ModPlatform::IndexedPack& pack, QJsonArray& arr);
ModPlatform::IndexedVersion loadIndexedPackVersion(QJsonObject& obj, bool load_changelog = false);
}  // namespace FlameMod
