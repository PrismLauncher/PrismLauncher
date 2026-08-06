// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/pages/modplatform/TexturePackModel.h"

namespace ResourceDownload {

class FlameTexturePackModel : public TexturePackResourceModel {
    Q_OBJECT

   public:
    FlameTexturePackModel(const BaseInstance&);
    ~FlameTexturePackModel() override = default;

    bool optedOut(const ModPlatform::IndexedVersion& ver) const override;

   private:
    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(const QModelIndex&) override;
};

}  // namespace ResourceDownload
