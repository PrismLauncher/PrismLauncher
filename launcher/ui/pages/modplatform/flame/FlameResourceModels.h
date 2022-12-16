#pragma once

#include "modplatform/flame/FlameAPI.h"

#include "ui/pages/modplatform/ModModel.h"

#include "ui/pages/modplatform/flame/FlameResourcePages.h"

namespace ResourceDownload {

class FlameModModel : public ModModel {
    Q_OBJECT

   public:
    FlameModModel(FlameModPage* parent);
    ~FlameModModel() override = default;

   private:
    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;

    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static const char* sorts[6]; 
    inline auto getSorts() const -> const char** override { return sorts; };
};

}  // namespace ResourceDownload
