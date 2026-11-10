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
                            const ResourceAPI* api,
                            TexturePackResourceModel* model = nullptr);

    static PageInfo page(ResourceDownloadDialog* dialog,
                         BaseInstance& instance,
                         ResourceProviderData provider,
                         const ResourceAPI* api,
                         TexturePackResourceModel* model,
                         bool suppressInitialSearch)
    {
        return {
            .id = provider.id,
            .displayName = provider.displayName,
            .icon = provider.icon,
            .constructor =
                [dialog, &instance, provider = std::move(provider), api, model, suppressInitialSearch](const QWidget* /*parent*/) {
                    auto* result = new TexturePackResourcePage(dialog, instance, provider, api, model);
                    result->setSuppressInitialSearch(suppressInitialSearch);
                    return result;
                },
        };
    }

   protected slots:
    void triggerSearch() override;
};

}  // namespace ResourceDownload
