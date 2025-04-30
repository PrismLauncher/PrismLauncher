// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ShaderPackModel.h"

#include <QMessageBox>
#include "api/structures/Provider.h"
#include "api/structures/ResourceType.h"

namespace ResourceDownload {

ShaderPackResourceModel::ShaderPackResourceModel(BaseInstance const& base_inst, Platform::Provider provider, QString metaEntryBase)
    : ResourceModel(provider)
    , m_base_instance(base_inst)
    , m_debugName(Platform::ProviderUtils::readableName(provider) + " (Model)")
    , m_metaEntryBase(metaEntryBase)
{}

/******** Make data requests ********/

API::SearchArgs ShaderPackResourceModel::createSearchArguments()
{
    auto sort = getCurrentSortingMethodByIndex();
    return { Platform::ResourceType::ShaderPack, m_next_search_offset, m_search_term, sort };
}

API::VersionSearchArgs ShaderPackResourceModel::createVersionsArguments(const QModelIndex& entry)
{
    auto& pack = m_packs[entry.row()];
    return { *pack, {}, {}, Platform::ResourceType::ShaderPack };
}

API::ProjectInfoArgs ShaderPackResourceModel::createInfoArguments(const QModelIndex& entry)
{
    return { Platform::ResourceType::ShaderPack, m_packs[entry.row()] };
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
