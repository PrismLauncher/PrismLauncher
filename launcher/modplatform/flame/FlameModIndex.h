//
// Created by timoreo on 16/01/2022.
//

#pragma once

#include "modplatform/ModIndex.h"

#include <QNetworkAccessManager>
#include "BaseInstance.h"

namespace FlameMod {

void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj);
void loadURLs(ModPlatform::IndexedPack& m, QJsonObject& obj);
void loadBody(ModPlatform::IndexedPack& m, QJsonObject& obj);
void loadIndexedPackVersions(ModPlatform::IndexedPack& pack,
                             QJsonArray& arr,
                             const shared_qobject_ptr<QNetworkAccessManager>& network,
                             const BaseInstance* inst);
auto loadIndexedPackVersion(QJsonObject& obj, bool load_changelog = false) -> ModPlatform::IndexedVersion;
auto loadDependencyVersions(const ModPlatform::Dependency& m, QJsonArray& arr, const BaseInstance* inst) -> ModPlatform::IndexedVersion;
}  // namespace FlameMod