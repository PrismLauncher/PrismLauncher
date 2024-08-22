// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/pages/modplatform/ModModel.h"
#include "ui/pages/modplatform/ResourcePackModel.h"
#include "ui/pages/modplatform/flame/FlameResourcePages.h"

namespace ResourceDownload {

class FlameModModel : public ModModel {
    Q_OBJECT

   public:
    FlameModModel(BaseInstance&);
    ~FlameModModel() override = default;

    bool optedOut(const ModPlatform::IndexedVersion& ver) const override;

   private:
    [[nodiscard]] QString debugName() const override { return Flame::debugName() + " (Model)"; }
    [[nodiscard]] QString metaEntryBase() const override { return Flame::metaEntryBase(); }

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;
    auto loadDependencyVersions(const ModPlatform::Dependency& m, QJsonArray& arr) -> ModPlatform::IndexedVersion override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;
};

class FlameResourcePackModel : public ResourcePackResourceModel {
    Q_OBJECT

   public:
    FlameResourcePackModel(const BaseInstance&);
    ~FlameResourcePackModel() override = default;

    bool optedOut(const ModPlatform::IndexedVersion& ver) const override;

   private:
    [[nodiscard]] QString debugName() const override { return Flame::debugName() + " (Model)"; }
    [[nodiscard]] QString metaEntryBase() const override { return Flame::metaEntryBase(); }

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;
};

class FlameTexturePackModel : public TexturePackResourceModel {
    Q_OBJECT

   public:
    FlameTexturePackModel(const BaseInstance&);
    ~FlameTexturePackModel() override = default;

    bool optedOut(const ModPlatform::IndexedVersion& ver) const override;

   private:
    [[nodiscard]] QString debugName() const override { return Flame::debugName() + " (Model)"; }
    [[nodiscard]] QString metaEntryBase() const override { return Flame::metaEntryBase(); }

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;

    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(QModelIndex&) override;

    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;
};

class FlameShaderPackModel : public ShaderPackResourceModel {
    Q_OBJECT

   public:
    FlameShaderPackModel(const BaseInstance&);
    ~FlameShaderPackModel() override = default;

    bool optedOut(const ModPlatform::IndexedVersion& ver) const override;

   private:
    [[nodiscard]] QString debugName() const override { return Flame::debugName() + " (Model)"; }
    [[nodiscard]] QString metaEntryBase() const override { return Flame::metaEntryBase(); }

    void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) override;
    void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) override;
    auto documentToArray(QJsonDocument& obj) const -> QJsonArray override;
};

}  // namespace ResourceDownload
