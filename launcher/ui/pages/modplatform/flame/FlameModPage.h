#pragma once

#include "ui/pages/modplatform/ModPage.h"

#include "modplatform/flame/FlameAPI.h"

class FlameModPage : public ModPage {
    Q_OBJECT

   public:
    explicit FlameModPage(ModDownloadDialog* dialog, BaseInstance* instance);
    virtual ~FlameModPage() = default;

    inline QString displayName() const override { return tr("CurseForge"); }
    inline QIcon icon() const override { return APPLICATION->getThemedIcon("flame"); }
    inline QString id() const override { return "curseforge"; }
    inline QString helpPage() const override { return "Flame-platform"; }

    inline QString debugName() const override { return tr("Flame"); }
    inline QString metaEntryBase() const override { return "FlameMods"; };

    bool shouldDisplay() const override;

   private:
    void onRequestVersionsSucceeded(QJsonDocument&, QString) override;
};
