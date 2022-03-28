#pragma once

#include "modplatform/ModIndex.h"

#include "BaseInstance.h"
#include <QNetworkAccessManager>

namespace Modrinth {

void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj);
void loadIndexedPackVersions(ModPlatform::IndexedPack& pack,
                             QJsonArray& arr,
                             const shared_qobject_ptr<QNetworkAccessManager>& network,
                             BaseInstance* inst);

}  // namespace Modrinth
