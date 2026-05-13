// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/pages/modplatform/ResourcePage.h"

namespace ResourceDownload {

class TexturePackDownloadDialog;
class TexturePackResourceModel;

class TexturePackResourcePage : public ResourcePage {
    Q_OBJECT

   public:
    TexturePackResourcePage(ResourceDownloadDialog* dialog,
                            BaseInstance& instance,
                            ResourceProviderData provider,
                            ResourceAPI* api,
                            TexturePackResourceModel* model = nullptr);

   protected slots:
    void triggerSearch() override;
};

}  // namespace ResourceDownload
