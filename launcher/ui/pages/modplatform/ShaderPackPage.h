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
    ShaderPackResourcePage(ResourceDownloadDialog* dialog, BaseInstance& instance, ResourceProviderData provider, const ResourceAPI* api);

    static PageInfo page(ResourceDownloadDialog* dialog, BaseInstance& instance, ResourceProviderData provider, const ResourceAPI* api, bool suppressInitialSearch)
    {
        return {
            .id = provider.id,
            .displayName = provider.displayName,
            .icon = provider.icon,
            .constructor =
                [dialog, &instance, provider = std::move(provider), api, suppressInitialSearch](const QWidget* /*parent*/) {
                    auto* result = new ShaderPackResourcePage(dialog, instance, provider, api);
                    result->setSuppressInitialSearch(suppressInitialSearch);
                    return result;
                },
        };
    }

   protected slots:
    void triggerSearch() override;
};

}  // namespace ResourceDownload
