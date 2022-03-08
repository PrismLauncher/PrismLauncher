#pragma once

#include "ui/pages/modplatform/ModPage.h"

#include "modplatform/flame/FlameAPI.h"

class FlameModPage : public ModPage {
    Q_OBJECT

   public:
    explicit FlameModPage(ModDownloadDialog* dialog, BaseInstance* instance);
    ~FlameModPage() override = default;

    inline auto displayName() const -> QString override { return tr("CurseForge"); }
    inline auto icon() const -> QIcon override { return APPLICATION->getThemedIcon("flame"); }
    inline auto id() const -> QString override { return "curseforge"; }
    inline auto helpPage() const -> QString override { return "Flame-platform"; }

    inline auto debugName() const -> QString override { return tr("Flame"); }
    inline auto metaEntryBase() const -> QString override { return "FlameMods"; };

    auto validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, QString loaderVer = "") const -> bool override;

    auto shouldDisplay() const -> bool override;
};
