#pragma once

#include "FlameModPage.h"
#include "modplatform/flame/FlameModIndex.h"

namespace FlameMod {

class ListModel : public ModPlatform::ListModel {
    Q_OBJECT

   public:
    ListModel(FlameModPage* parent) : ModPlatform::ListModel(parent) {}
;
    virtual ~ListModel() = default;

   private:
    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override { FlameMod::loadIndexedPack(m, obj); };

    QJsonArray documentToArray(QJsonDocument& obj) const override { return obj.array(); };

    static const char* sorts[6]; 
    const char** getSorts() const override { return sorts; };
};

}  // namespace FlameMod
