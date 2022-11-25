#pragma once

#include <QWidget>

#include "modplatform/ModIndex.h"

#include "ui/pages/modplatform/ResourcePage.h"
#include "ui/widgets/ModFilterWidget.h"

class ModDownloadDialog;

namespace Ui {
class ResourcePage;
}

/* This page handles most logic related to browsing and selecting mods to download. */
class ModPage : public ResourcePage {
    Q_OBJECT

   public:
    template<typename T>
    static T* create(ModDownloadDialog* dialog, BaseInstance& instance)
    {
        auto page = new T(dialog, instance);

        auto filter_widget = ModFilterWidget::create(static_cast<MinecraftInstance&>(instance).getPackProfile()->getComponentVersion("net.minecraft"), page);
        page->setFilterWidget(filter_widget);

        return page;
    }

    ~ModPage() override = default;

    [[nodiscard]] inline QString resourceString() const override { return tr("mod"); }

    [[nodiscard]] QMap<QString, QString> urlHandlers() const override;

    void addResourceToDialog(ModPlatform::IndexedPack&, ModPlatform::IndexedVersion&) override;

    virtual auto validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, std::optional<ResourceAPI::ModLoaderTypes> loaders = {}) const -> bool = 0;

    [[nodiscard]] bool supportsFiltering() const override { return true; };
    auto getFilter() const -> const std::shared_ptr<ModFilterWidget::Filter> { return m_filter; }
    void setFilterWidget(unique_qobject_ptr<ModFilterWidget>&);

   public slots:
    void updateVersionList() override;

   protected:
    ModPage(ModDownloadDialog* dialog, BaseInstance& instance);

   protected slots:
    virtual void filterMods();
    void triggerSearch() override;

   protected:
    unique_qobject_ptr<ModFilterWidget> m_filter_widget;
    std::shared_ptr<ModFilterWidget::Filter> m_filter;
};
