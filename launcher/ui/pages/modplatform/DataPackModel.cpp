// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
// SPDX-FileCopyrightText: 2023 TheKodeToad <TheKodeToad@proton.me>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "DataPackModel.h"

#include <QMessageBox>

namespace ResourceDownload {

DataPackResourceModel::DataPackResourceModel(const BaseInstance& base_inst,
                                             const ResourceAPI* api,
                                             QString debugName,
                                             QString metaEntryBase)
    : ResourceModel(api), m_base_instance(base_inst), m_debugName(debugName + " (Model)"), m_metaEntryBase(metaEntryBase)
{}

/******** Make data requests ********/

ResourceAPI::SearchArgs DataPackResourceModel::createSearchArguments()
{
    auto sort = getCurrentSortingMethodByIndex();
    return { ModPlatform::ResourceType::DataPack, m_nextSearchOffset, m_searchTerm, sort, ModPlatform::ModLoaderType::DataPack };
}

ResourceAPI::VersionSearchArgs DataPackResourceModel::createVersionsArguments(const QModelIndex& entry)
{
    auto pack = m_packs[entry.row()];
    return { pack, {}, ModPlatform::ModLoaderType::DataPack };
}

ResourceAPI::ProjectInfoArgs DataPackResourceModel::createInfoArguments(const QModelIndex& entry)
{
    auto pack = m_packs[entry.row()];
    return { pack };
}

void DataPackResourceModel::searchWithTerm(const QString& term, unsigned int sort)
{
    if (m_searchTerm == term && m_searchTerm.isNull() == term.isNull() && m_currentSortIndex == sort) {
        return;
    }

    setSearchTerm(term);
    m_currentSortIndex = sort;

    refresh();
}

}  // namespace ResourceDownload
