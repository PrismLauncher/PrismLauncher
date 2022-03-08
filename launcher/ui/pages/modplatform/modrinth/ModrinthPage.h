#pragma once

#include "ui/pages/modplatform/ModPage.h"

#include "modplatform/modrinth/ModrinthAPI.h"

class ModrinthPage : public ModPage {
    Q_OBJECT

   public:
    explicit ModrinthPage(ModDownloadDialog* dialog, BaseInstance* instance);
    ~ModrinthPage() override = default;

    inline auto displayName() const -> QString override { return tr("Modrinth"); }
    inline auto icon() const -> QIcon override { return APPLICATION->getThemedIcon("modrinth"); }
    inline auto id() const -> QString override { return "modrinth"; }
    inline auto helpPage() const -> QString override { return "Modrinth-platform"; }

    inline auto debugName() const -> QString override { return tr("Modrinth"); }
    inline auto metaEntryBase() const -> QString override { return "ModrinthPacks"; };

    auto validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, QString loaderVer = "") const -> bool override;

    auto shouldDisplay() const -> bool override;
};
