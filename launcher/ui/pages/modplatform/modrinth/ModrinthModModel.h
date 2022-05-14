#pragma once

#include "ModrinthModPage.h"

namespace Modrinth {

class ListModel : public ModPlatform::ListModel {
    Q_OBJECT

   public:
    ListModel(ModrinthModPage* parent) : ModPlatform::ListModel(parent){};
    ~ListModel() override = default;

   private:
    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;
    
    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;

    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static const char* sorts[5];
    inline auto getSorts() const -> const char** override { return sorts; };
};

}  // namespace Modrinth
