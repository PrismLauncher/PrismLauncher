// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QWidget>

#include "modplatform/ModIndex.h"

#include "ui/pages/modplatform/ModModel.h"
#include "ui/pages/modplatform/ResourcePage.h"
#include "ui/widgets/ModFilterWidget.h"

namespace Ui {
class ResourcePage;
}

namespace ResourceDownload {

class ModDownloadDialog;

/* This page handles most logic related to browsing and selecting mods to download. */
class ModPage : public ResourcePage {
    Q_OBJECT

   public:
    template <typename T>
    static T* create(ModDownloadDialog* dialog, BaseInstance& instance)
    {
        auto page = new T(dialog, instance);
        auto model = static_cast<ModModel*>(page->getModel());

        auto filter_widget =
            ModFilterWidget::create(static_cast<MinecraftInstance&>(instance).getPackProfile()->getComponentVersion("net.minecraft"), page);
        page->setFilterWidget(filter_widget);
        model->setFilter(page->getFilter());

        connect(model, &ResourceModel::versionListUpdated, page, &ResourcePage::updateVersionList);
        connect(model, &ResourceModel::projectInfoUpdated, page, &ResourcePage::updateUi);

        return page;
    }

    //: The plural version of 'mod'
    [[nodiscard]] inline QString resourcesString() const override { return tr("mods"); }
    //: The singular version of 'mods'
    [[nodiscard]] inline QString resourceString() const override { return tr("mod"); }

    [[nodiscard]] QMap<QString, QString> urlHandlers() const override;

    void addResourceToPage(ModPlatform::IndexedPack::Ptr,
                           ModPlatform::IndexedVersion&,
                           const std::shared_ptr<ResourceFolderModel>) override;

    virtual auto validateVersion(ModPlatform::IndexedVersion& ver,
                                 QString mineVer,
                                 std::optional<ModPlatform::ModLoaderTypes> loaders = {}) const -> bool = 0;

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

}  // namespace ResourceDownload
