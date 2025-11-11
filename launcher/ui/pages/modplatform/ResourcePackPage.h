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

        connect(model, &ResourceModel::versionListUpdated, page, &ResourcePage::versionListUpdated);
        connect(model, &ResourceModel::projectInfoUpdated, page, &ResourcePage::updateUi);
        connect(model, &QAbstractListModel::modelReset, page, &ResourcePage::modelReset);

        return page;
    }

    //: The plural version of 'resource pack'
    inline QString resourcesString() const override { return tr("resource packs"); }
    //: The singular version of 'resource packs'
    inline QString resourceString() const override { return tr("resource pack"); }

    bool supportsFiltering() const override { return false; };

    QMap<QString, QString> urlHandlers() const override;

    inline auto helpPage() const -> QString override { return "resourcepack-platform"; }

   protected:
    ResourcePackResourcePage(ResourceDownloadDialog* dialog, BaseInstance& instance);

   protected slots:
    void triggerSearch() override;
};

}  // namespace ResourceDownload
