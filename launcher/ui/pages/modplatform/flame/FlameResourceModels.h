// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/pages/modplatform/ModModel.h"
#include "ui/pages/modplatform/flame/FlameResourcePages.h"

namespace ResourceDownload {

class FlameModModel : public ModModel {
    Q_OBJECT

   public:
    FlameModModel(const BaseInstance&);
    ~FlameModModel() override = default;

   private:
    [[nodiscard]] QString debugName() const override { return Flame::debugName() + " (Model)"; }
    [[nodiscard]] QString metaEntryBase() const override { return Flame::metaEntryBase(); }

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;
};

}  // namespace ResourceDownload
