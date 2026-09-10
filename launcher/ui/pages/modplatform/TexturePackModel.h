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
    TexturePackResourceModel(const BaseInstance& inst, const ResourceAPI* api, const QString& debugName, QString metaEntryBase);

    inline ::Version maximumTexturePackVersion() const { return { "1.6" }; }

    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(const QModelIndex&) override;

   protected:
    // Texture packs (pre-1.6) are matched by pack-format compatibility rather than an exact
    // Minecraft version, which is already handled by restricting the searched/requested
    // versions to s_availableVersions above, so fall back to the base (opt-out only) check here.
    bool checkVersionFilters(const ModPlatform::IndexedVersion& v) override { return ResourceModel::checkVersionFilters(v); }

   protected:
    Meta::VersionList::Ptr m_version_list;
    Task::Ptr m_task;
};

}  // namespace ResourceDownload
