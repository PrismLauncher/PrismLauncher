// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/pages/modplatform/ResourcePage.h"

namespace ResourceDownload {

class ResourcePackDownloadDialog;

class ResourcePackResourcePage : public ResourcePage {
    Q_OBJECT

   public:
    ResourcePackResourcePage(ResourceDownloadDialog* dialog, BaseInstance& instance, ResourceProviderData provider, const ResourceAPI* api);

   protected slots:
    void triggerSearch() override;
};

}  // namespace ResourceDownload
