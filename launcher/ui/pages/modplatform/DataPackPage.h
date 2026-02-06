// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
// SPDX-FileCopyrightText: 2023 TheKodeToad <TheKodeToad@proton.me>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/pages/modplatform/ResourcePage.h"

namespace ResourceDownload {

class DataPackDownloadDialog;
class DataPackResourcePage : public ResourcePage {
    Q_OBJECT

   public:
    DataPackResourcePage(DataPackDownloadDialog* dialog, BaseInstance& instance, ResourceProviderData p, ResourceAPI* api);

   protected slots:
    void triggerSearch() override;
};

}  // namespace ResourceDownload
