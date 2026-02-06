// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QWidget>

#include "ui/pages/modplatform/ModModel.h"
#include "ui/pages/modplatform/ResourcePage.h"
#include "ui/widgets/ModFilterWidget.h"

namespace ResourceDownload {

class ModDownloadDialog;

/* This page handles most logic related to browsing and selecting mods to download. */
class ModPage : public ResourcePage {
    Q_OBJECT

   public:
    auto getFilter() const -> const std::shared_ptr<ModFilterWidget::Filter> { return m_filter; }

    ModPage(ModDownloadDialog* dialog, BaseInstance& instance, ResourceProviderData p, ResourceAPI* api, ModFilterWidget* filterWidget);

   protected:
    void prepareProviderCategories();
    void setFilterWidget(ModFilterWidget*);

   protected slots:
    virtual void filterMods();
    void triggerSearch() override;

   protected:
    std::unique_ptr<ModFilterWidget> m_filter_widget;
    std::shared_ptr<ModFilterWidget::Filter> m_filter;
    Task::Ptr m_categoriesTask;
    ResourceAPI* m_api = nullptr;
};

}  // namespace ResourceDownload
