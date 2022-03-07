#pragma once

#include "ModrinthPage.h"

namespace Modrinth {

class ListModel : public ModPlatform::ListModel {
    Q_OBJECT

   public:
    ListModel(ModrinthPage* parent) : ModPlatform::ListModel(parent){};
    virtual ~ListModel() = default;

   private:
    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;
    
    QJsonArray documentToArray(QJsonDocument& obj) const override;

    static const char* sorts[5];
    inline const char** getSorts() const override { return sorts; };
};

}  // namespace Modrinth
