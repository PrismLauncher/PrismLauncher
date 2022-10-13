#pragma once

#include "BaseInstance.h"

#include "modplatform/modrinth/ModrinthAPI.h"
#include "modplatform/modrinth/ModrinthPackManifest.h"

#include "ui/pages/BasePage.h"

#include <QWidget>

namespace Ui {
class ManagedPackPage;
}

class ManagedPackPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    inline static ManagedPackPage* createPage(BaseInstance* inst, QWidget* parent = nullptr)
    {
        return ManagedPackPage::createPage(inst, inst->getManagedPackType(), parent);
    }

    static ManagedPackPage* createPage(BaseInstance* inst, QString type, QWidget* parent = nullptr);
    ~ManagedPackPage() override;

    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] QIcon icon() const override;
    [[nodiscard]] QString helpPage() const override;
    [[nodiscard]] QString id() const override { return "managed_pack"; }
    [[nodiscard]] bool shouldDisplay() const override;

    void openedImpl() override;

    bool apply() override { return true; }
    void retranslate() override;

    /** Gets the necessary information about the managed pack, such as
     *  available versions*/
    virtual void parseManagedPack() {};

    /** URL of the managed pack.
     *  Not the version-specific one.
     */
    [[nodiscard]] virtual QString url() const { return {}; };

   public slots:
    /** Gets the current version selection and update the changelog.
     */
    virtual void suggestVersion() {};

   protected:
    ManagedPackPage(BaseInstance* inst, QWidget* parent = nullptr);

   protected:
    Ui::ManagedPackPage* ui;
    BaseInstance* m_inst;

    bool m_loaded = false;
};

/** Simple page for when we aren't a managed pack. */
class GenericManagedPackPage final : public ManagedPackPage {
    Q_OBJECT

   public:
    GenericManagedPackPage(BaseInstance* inst, QWidget* parent = nullptr) : ManagedPackPage(inst, parent) {}
    ~GenericManagedPackPage() override = default;
};

class ModrinthManagedPackPage final : public ManagedPackPage {
    Q_OBJECT

   public:
    ModrinthManagedPackPage(BaseInstance* inst, QWidget* parent = nullptr);
    ~ModrinthManagedPackPage() override = default;

    void parseManagedPack() override;
    [[nodiscard]] QString url() const override;

   public slots:
    void suggestVersion() override;
};

class FlameManagedPackPage final : public ManagedPackPage {
    Q_OBJECT

   public:
    FlameManagedPackPage(BaseInstance* inst, QWidget* parent = nullptr);
    ~FlameManagedPackPage() override = default;

    void parseManagedPack() override;
    [[nodiscard]] QString url() const override;

   public slots:
    void suggestVersion() override;
};
