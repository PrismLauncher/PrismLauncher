// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ShaderPackModel.h"

#include <QMessageBox>

namespace ResourceDownload {

ShaderPackResourceModel::ShaderPackResourceModel(BaseInstance const& base_inst, ResourceAPI* api)
    : ResourceModel(api), m_base_instance(base_inst){};

/******** Make data requests ********/

ResourceAPI::SearchArgs ShaderPackResourceModel::createSearchArguments()
{
    auto sort = getCurrentSortingMethodByIndex();
    return { ModPlatform::ResourceType::SHADER_PACK, m_next_search_offset, m_search_term, sort };
}

ResourceAPI::VersionSearchArgs ShaderPackResourceModel::createVersionsArguments(QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { *pack };
}

ResourceAPI::ProjectInfoArgs ShaderPackResourceModel::createInfoArguments(QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { *pack };
}

void ShaderPackResourceModel::searchWithTerm(const QString& term, unsigned int sort)
{
    if (m_search_term == term && m_search_term.isNull() == term.isNull() && m_current_sort_index == sort) {
        return;
    }

    setSearchTerm(term);
    m_current_sort_index = sort;

    refresh();
}

}  // namespace ResourceDownload
