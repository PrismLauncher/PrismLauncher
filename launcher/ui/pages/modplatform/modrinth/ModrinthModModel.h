// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
