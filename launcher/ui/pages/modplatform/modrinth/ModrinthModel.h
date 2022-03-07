#pragma once

#include "ModrinthPage.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

namespace Modrinth {

class ListModel : public ModPlatform::ListModel {
    Q_OBJECT

   public:
    ListModel(ModrinthPage* parent) : ModPlatform::ListModel(parent){};
    virtual ~ListModel() = default;

   private:
    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override { Modrinth::loadIndexedPack(m, obj); };

    QJsonArray documentToArray(QJsonDocument& obj) const override { return obj.object().value("hits").toArray(); };

    static const char* sorts[5];
    const char** getSorts() const override { return sorts; };
};

}  // namespace Modrinth
