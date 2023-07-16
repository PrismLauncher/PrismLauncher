// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
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

#include "ui/pages/modplatform/ModModel.h"
#include "ui/pages/modplatform/ResourcePackModel.h"
#include "ui/pages/modplatform/modrinth/ModrinthResourcePages.h"

namespace ResourceDownload {

class ModrinthModModel : public ModModel {
    Q_OBJECT

   public:
    ModrinthModModel(BaseInstance&);
    ~ModrinthModModel() override = default;

   private:
    [[nodiscard]] QString debugName() const override { return Modrinth::debugName() + " (Model)"; }
    [[nodiscard]] QString metaEntryBase() const override { return Modrinth::metaEntryBase(); }

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;
    auto loadDependencyVersions(const ModPlatform::Dependency& m, QJsonArray& arr) -> ModPlatform::IndexedVersion override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;
};

class ModrinthResourcePackModel : public ResourcePackResourceModel {
    Q_OBJECT

   public:
    ModrinthResourcePackModel(const BaseInstance&);
    ~ModrinthResourcePackModel() override = default;

   private:
    [[nodiscard]] QString debugName() const override { return Modrinth::debugName() + " (Model)"; }
    [[nodiscard]] QString metaEntryBase() const override { return Modrinth::metaEntryBase(); }

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;
};

class ModrinthTexturePackModel : public TexturePackResourceModel {
    Q_OBJECT

   public:
    ModrinthTexturePackModel(const BaseInstance&);
    ~ModrinthTexturePackModel() override = default;

   private:
    [[nodiscard]] QString debugName() const override { return Modrinth::debugName() + " (Model)"; }
    [[nodiscard]] QString metaEntryBase() const override { return Modrinth::metaEntryBase(); }

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;
};

class ModrinthShaderPackModel : public ShaderPackResourceModel {
    Q_OBJECT

   public:
    ModrinthShaderPackModel(const BaseInstance&);
    ~ModrinthShaderPackModel() override = default;

   private:
    [[nodiscard]] QString debugName() const override { return Modrinth::debugName() + " (Model)"; }
    [[nodiscard]] QString metaEntryBase() const override { return Modrinth::metaEntryBase(); }

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;
};

}  // namespace ResourceDownload
