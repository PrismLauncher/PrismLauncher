// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
// SPDX-FileCopyrightText: 2023 TheKodeToad <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "DataPackModel.h"

#include <QMessageBox>

namespace ResourceDownload {

DataPackResourceModel::DataPackResourceModel(BaseInstance const& base_inst, ResourceAPI* api)
    : ResourceModel(api), m_base_instance(base_inst)
{}

/******** Make data requests ********/

ResourceAPI::SearchArgs DataPackResourceModel::createSearchArguments()
{
    auto sort = getCurrentSortingMethodByIndex();
    return { ModPlatform::ResourceType::DATA_PACK, m_next_search_offset, m_search_term, sort };
}

ResourceAPI::VersionSearchArgs DataPackResourceModel::createVersionsArguments(QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { *pack };
}

ResourceAPI::ProjectInfoArgs DataPackResourceModel::createInfoArguments(QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { *pack };
}

void DataPackResourceModel::searchWithTerm(const QString& term, unsigned int sort)
{
    if (m_search_term == term && m_search_term.isNull() == term.isNull() && m_current_sort_index == sort) {
        return;
    }

    setSearchTerm(term);
    m_current_sort_index = sort;

    refresh();
}

}  // namespace ResourceDownload
