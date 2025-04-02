// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ResourcePackModel.h"

#include <QMessageBox>
#include "api/structures/ResourceType.h"

namespace ResourceDownload {

ResourcePackResourceModel::ResourcePackResourceModel(BaseInstance const& base_inst,
                                                     ResourceAPI* api,
                                                     QString debugName,
                                                     QString metaEntryBase)
    : ResourceModel(api), m_base_instance(base_inst), m_debugName(debugName + " (Model)"), m_metaEntryBase(metaEntryBase)
{}

/******** Make data requests ********/

API::SearchArgs ResourcePackResourceModel::createSearchArguments()
{
    auto sort = getCurrentSortingMethodByIndex();
    return { Platform::ResourceType::ResourcePack, m_next_search_offset, m_search_term, sort };
}

API::VersionSearchArgs ResourcePackResourceModel::createVersionsArguments(const QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { *pack, {}, {}, Platform::ResourceType::ResourcePack };
}

API::ProjectInfoArgs ResourcePackResourceModel::createInfoArguments(const QModelIndex& entry)
{
    return { Platform::ResourceType::ResourcePack, m_packs[entry.row()] };
}

void ResourcePackResourceModel::searchWithTerm(const QString& term, unsigned int sort)
{
    if (m_search_term == term && m_search_term.isNull() == term.isNull() && m_current_sort_index == sort) {
        return;
    }

    setSearchTerm(term);
    m_current_sort_index = sort;

    refresh();
}

}  // namespace ResourceDownload
