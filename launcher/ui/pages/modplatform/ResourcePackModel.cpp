// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ResourcePackModel.h"

#include <QMessageBox>

namespace ResourceDownload {

ResourcePackResourceModel::ResourcePackResourceModel(BaseInstance const& base_inst, ResourceAPI* api)
    : ResourceModel(api), m_base_instance(base_inst){};

/******** Make data requests ********/

ResourceAPI::SearchArgs ResourcePackResourceModel::createSearchArguments()
{
    auto sort = getCurrentSortingMethodByIndex();
    return { ModPlatform::ResourceType::RESOURCE_PACK, m_next_search_offset, m_search_term, sort };
}

ResourceAPI::VersionSearchArgs ResourcePackResourceModel::createVersionsArguments(QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { *pack };
}

ResourceAPI::ProjectInfoArgs ResourcePackResourceModel::createInfoArguments(QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { *pack };
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
