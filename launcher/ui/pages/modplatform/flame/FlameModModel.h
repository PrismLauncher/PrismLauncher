#pragma once

#include "FlameModPage.h"

namespace FlameMod {

class ListModel : public ModPlatform::ListModel {
    Q_OBJECT

   public:
    ListModel(FlameModPage* parent) : ModPlatform::ListModel(parent) {}
    ~ListModel() override = default;

   private:
    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;

    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static const char* sorts[6]; 
    inline auto getSorts() const -> const char** override { return sorts; };
};

}  // namespace FlameMod
