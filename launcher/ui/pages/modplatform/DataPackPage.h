// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
// SPDX-FileCopyrightText: 2023 TheKodeToad <TheKodeToad@proton.me>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/pages/modplatform/DataPackModel.h"
#include "ui/pages/modplatform/ResourcePage.h"

namespace Ui {
class ResourcePage;
}

namespace ResourceDownload {

class DataPackDownloadDialog;

class DataPackResourcePage : public ResourcePage {
    Q_OBJECT

   public:
    template <typename T>
    static T* create(DataPackDownloadDialog* dialog, BaseInstance& instance)
    {
        auto page = new T(dialog, instance);
        auto model = static_cast<DataPackResourceModel*>(page->getModel());

        connect(model, &ResourceModel::versionListUpdated, page, &ResourcePage::versionListUpdated);
        connect(model, &ResourceModel::projectInfoUpdated, page, &ResourcePage::updateUi);
        connect(model, &QAbstractListModel::modelReset, page, &ResourcePage::modelReset);

        return page;
    }

    //: The plural version of 'data pack'
    inline QString resourcesString() const override { return tr("data packs"); }
    //: The singular version of 'data packs'
    inline QString resourceString() const override { return tr("data pack"); }

    bool supportsFiltering() const override { return false; };

    QMap<QString, QString> urlHandlers() const override;

   protected:
    DataPackResourcePage(DataPackDownloadDialog* dialog, BaseInstance& instance);

   protected slots:
    void triggerSearch() override;
};

}  // namespace ResourceDownload
