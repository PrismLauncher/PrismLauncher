// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ui/pages/modplatform/ResourcePage.h"

namespace ResourceDownload {

class ResourceDownloadDialog;

class ShaderPackResourcePage : public ResourcePage {
    Q_OBJECT

   public:
    ShaderPackResourcePage(ResourceDownloadDialog* dialog, BaseInstance& instance, ResourceProviderData p, ResourceAPI* api);

   protected slots:
    void triggerSearch() override;
};

}  // namespace ResourceDownload
