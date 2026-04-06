// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ShaderPackModel.h"

#include <QMessageBox>
#include <utility>

namespace ResourceDownload {

ShaderPackResourceModel::ShaderPackResourceModel(const BaseInstance& base_inst,
                                                 ResourceAPI* api,
                                                 const QString& debugName,
                                                 QString metaEntryBase)
    : ResourceModel(api), m_base_instance(base_inst), m_debugName(debugName + " (Model)"), m_metaEntryBase(std::move(metaEntryBase))
{}

/******** Make data requests ********/

ResourceAPI::SearchArgs ShaderPackResourceModel::createSearchArguments()
{
    auto sort = getCurrentSortingMethodByIndex();
    return {
        .type = ModPlatform::ResourceType::ShaderPack,
        .offset = m_next_search_offset,
        .search = m_search_term,
        .sorting = sort,
        .loaders = {},
        .versions = {},
        .side = {},
        .categoryIds = {},
        .openSource = {},
    };
}

ResourceAPI::VersionSearchArgs ShaderPackResourceModel::createVersionsArguments(const QModelIndex& entry)
{
    auto pack = m_packs[entry.row()];
    return { .pack = pack, .mcVersions = {}, .loaders = {}, .resourceType = ModPlatform::ResourceType::ShaderPack };
}

ResourceAPI::ProjectInfoArgs ShaderPackResourceModel::createInfoArguments(const QModelIndex& entry)
{
    auto pack = m_packs[entry.row()];
    return { .pack = pack };
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
