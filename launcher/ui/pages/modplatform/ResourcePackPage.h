// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/pages/modplatform/ResourcePackModel.h"
#include "ui/pages/modplatform/ResourcePage.h"

namespace Ui {
class ResourcePage;
}

namespace ResourceDownload {

class ResourcePackDownloadDialog;

class ResourcePackResourcePage : public ResourcePage {
    Q_OBJECT

   public:
    template <typename T>
    static T* create(ResourcePackDownloadDialog* dialog, BaseInstance& instance)
    {
        auto page = new T(dialog, instance);
        auto model = static_cast<ResourcePackResourceModel*>(page->getModel());

        connect(model, &ResourceModel::versionListUpdated, page, &ResourcePage::updateVersionList);
        connect(model, &ResourceModel::projectInfoUpdated, page, &ResourcePage::updateUi);

        return page;
    }

    //: The plural version of 'resource pack'
    [[nodiscard]] inline QString resourcesString() const override { return tr("resource packs"); }
    //: The singular version of 'resource packs'
    [[nodiscard]] inline QString resourceString() const override { return tr("resource pack"); }

    [[nodiscard]] bool supportsFiltering() const override { return false; };

    [[nodiscard]] QMap<QString, QString> urlHandlers() const override;

    [[nodiscard]] inline auto helpPage() const -> QString override { return "resourcepack-platform"; }

   protected:
    ResourcePackResourcePage(ResourceDownloadDialog* dialog, BaseInstance& instance);

   protected slots:
    void triggerSearch() override;
};

}  // namespace ResourceDownload
