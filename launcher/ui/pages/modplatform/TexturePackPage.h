// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/pages/modplatform/ResourcePackPage.h"
#include "ui/pages/modplatform/TexturePackModel.h"
#include "ui_ResourcePage.h"

namespace Ui {
class ResourcePage;
}

namespace ResourceDownload {

class TexturePackDownloadDialog;

class TexturePackResourcePage : public ResourcePackResourcePage {
    Q_OBJECT

   public:
    template <typename T>
    static T* create(TexturePackDownloadDialog* dialog, BaseInstance& instance)
    {
        auto page = new T(dialog, instance);
        auto model = static_cast<TexturePackResourceModel*>(page->getModel());

        connect(model, &ResourceModel::versionListUpdated, page, &ResourcePage::updateVersionList);
        connect(model, &ResourceModel::projectInfoUpdated, page, &ResourcePage::updateUi);

        return page;
    }

    //: The plural version of 'texture pack'
    [[nodiscard]] inline QString resourcesString() const override { return tr("texture packs"); }
    //: The singular version of 'texture packs'
    [[nodiscard]] inline QString resourceString() const override { return tr("texture pack"); }

   protected:
    TexturePackResourcePage(TexturePackDownloadDialog* dialog, BaseInstance& instance) : ResourcePackResourcePage(dialog, instance)
    {
        connect(m_ui->packView, &QListView::doubleClicked, this, &TexturePackResourcePage::onResourceSelected);
    }
};

}  // namespace ResourceDownload
