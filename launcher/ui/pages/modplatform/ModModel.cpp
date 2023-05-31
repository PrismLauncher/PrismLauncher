// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModModel.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include <QMessageBox>

namespace ResourceDownload {

ModModel::ModModel(BaseInstance const& base_inst, ResourceAPI* api) : ResourceModel(api), m_base_instance(base_inst) {}

/******** Make data requests ********/

ResourceAPI::SearchArgs ModModel::createSearchArguments()
{
    auto profile = static_cast<MinecraftInstance const&>(m_base_instance).getPackProfile();

    Q_ASSERT(profile);
    Q_ASSERT(m_filter);

    std::optional<std::list<Version>> versions{};

    { // Version filter
        if (!m_filter->versions.empty())
            versions = m_filter->versions;
    }

    auto sort = getCurrentSortingMethodByIndex();

    return { ModPlatform::ResourceType::MOD, m_next_search_offset, m_search_term, sort, profile->getModLoaders(), versions };
}

ResourceAPI::VersionSearchArgs ModModel::createVersionsArguments(QModelIndex& entry)
{
    auto& pack = *m_packs[entry.row()];
    auto profile = static_cast<MinecraftInstance const&>(m_base_instance).getPackProfile();

    Q_ASSERT(profile);
    Q_ASSERT(m_filter);

    std::optional<std::list<Version>> versions{};
    if (!m_filter->versions.empty())
        versions = m_filter->versions;

    return { pack, versions, profile->getModLoaders() };
}

ResourceAPI::ProjectInfoArgs ModModel::createInfoArguments(QModelIndex& entry)
{
    auto& pack = *m_packs[entry.row()];
    return { pack };
}

void ModModel::searchWithTerm(const QString& term, unsigned int sort, bool filter_changed)
{
    if (m_search_term == term && m_search_term.isNull() == term.isNull() && m_current_sort_index == sort && !filter_changed) {
        return;
    }

    setSearchTerm(term);
    m_current_sort_index = sort;

    refresh();
}

}  // namespace ResourceDownload
