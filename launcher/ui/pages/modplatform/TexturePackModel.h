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
    TexturePackResourceModel(ResourceFolderModel* inst, const ResourceAPI* api, const QString& debugName, QString metaEntryBase);

    ResourceAPI::SearchArgs createSearchArguments() override;
    ResourceAPI::VersionSearchArgs createVersionsArguments(const QModelIndex& /*unused*/) override;

   protected:
    Meta::VersionList::Ptr m_versionList;
    Task::Ptr m_task;
};

}  // namespace ResourceDownload
