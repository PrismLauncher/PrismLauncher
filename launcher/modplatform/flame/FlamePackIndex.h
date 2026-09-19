//
// Created by timoreo on 16/01/2022.
//

#pragma once

#include "Result.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceType.h"

namespace Flame::Parse {
Result<> loadIndexedPack(ModPlatform::IndexedPack& pack, const QJsonObject& obj);
Result<QList<ModPlatform::IndexedVersion>> loadIndexedPackVersions(const QJsonArray& arr,
                                                                   const QString& addonId = {},
                                                                   ModPlatform::ResourceType resourceType = ModPlatform::ResourceType::Mod);
Result<ModPlatform::IndexedVersion> loadIndexedPackVersion(const QJsonObject& obj);

Result<QList<ModPlatform::IndexedPack>> parseProjectList(const QByteArray& response);

int getClassId(ModPlatform::ResourceType type);
}  // namespace Flame::Parse
