// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "meta/VersionList.h"
#include "ui/pages/modplatform/ResourcePackModel.h"

namespace ResourceDownload {

class TexturePackResourceModel : public ResourcePackResourceModel {
    Q_OBJECT

   public:
    TexturePackResourceModel(BaseInstance const& inst, ResourceAPI* api);

    [[nodiscard]] inline ::Version maximumTexturePackVersion() const { return { "1.6" }; }

    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(QModelIndex&) override;

   protected:
    Meta::VersionList::Ptr m_version_list;
    Task::Ptr m_task;
};

}  // namespace ResourceDownload
