// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
// SPDX-FileCopyrightText: 2023 TheKodeToad <TheKodeToad@proton.me>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "DataPackModel.h"

#include <QMessageBox>
#include <utility>

namespace ResourceDownload {

DataPackResourceModel::DataPackResourceModel(const BaseInstance& baseInst,
                                             const ResourceAPI* api,
                                             const QString& debugName,
                                             QString metaEntryBase)
    : ResourceModel(api), m_baseInstance(baseInst), m_debugName(debugName + " (Model)"), m_metaEntryBase(std::move(metaEntryBase))
{}

/******** Make data requests ********/

ResourceAPI::SearchArgs DataPackResourceModel::createSearchArguments()
{
    auto sort = getCurrentSortingMethodByIndex();
return { .type = ModPlatform::ResourceType::DataPack,
             .offset = m_nextSearchOffset,
             .search = m_searchTerm,
             .sorting = sort,
             .loaders = ModPlatform::ModLoaderType::DataPack };
}

ResourceAPI::VersionSearchArgs DataPackResourceModel::createVersionsArguments(const QModelIndex& entry)
{
    auto pack = m_packs[entry.row()];
    return { .pack = pack, .mcVersions = {}, .loaders = ModPlatform::ModLoaderType::DataPack };
}

QString DataPackResourceModel::createInfoArguments(const QModelIndex& entry)
{
    auto pack = m_packs[entry.row()];
    return pack->addonId.toString();
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
