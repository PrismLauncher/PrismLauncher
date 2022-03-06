#pragma once

#include "ui/pages/modplatform/ModPage.h"

#include "modplatform/modrinth/ModrinthAPI.h"

class ModrinthPage : public ModPage {
    Q_OBJECT

   public:
    explicit ModrinthPage(ModDownloadDialog* dialog, BaseInstance* instance);
    virtual ~ModrinthPage() = default;

    inline QString displayName() const override { return tr("Modrinth"); }
    inline QIcon icon() const override { return APPLICATION->getThemedIcon("modrinth"); }
    inline QString id() const override { return "modrinth"; }
    inline QString helpPage() const override { return "Modrinth-platform"; }

    inline QString debugName() const override { return tr("Modrinth"); }
    inline QString metaEntryBase() const override { return "ModrinthPacks"; };

    bool shouldDisplay() const override;

   private:
    void onGetVersionsSucceeded(ModPage*, QByteArray*, QString) override;
};
