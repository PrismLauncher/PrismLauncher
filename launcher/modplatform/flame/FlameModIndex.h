//
// Created by timoreo on 16/01/2022.
//

#pragma once

#include "api/structures/Project.h"

#include "BaseInstance.h"

namespace FlameMod {

void loadIndexedPack(Platform::Project& m, QJsonObject& obj);
void loadURLs(Platform::Project& m, QJsonObject& obj);
Platform::Version loadIndexedPackVersion(QJsonObject& obj, bool load_changelog = false);
}  // namespace FlameMod