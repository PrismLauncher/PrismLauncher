//
// Created by timoreo on 16/01/2022.
//

#pragma once

#include "Result.h"
#include "modplatform/ModIndex.h"

namespace Flame::Parse {
Result<> loadIndexedPack(ModPlatform::IndexedPack& pack, const QJsonObject& obj);

Result<QList<ModPlatform::IndexedPack>> parseProjectList(const QByteArray& response);
}  // namespace Flame::Parse
