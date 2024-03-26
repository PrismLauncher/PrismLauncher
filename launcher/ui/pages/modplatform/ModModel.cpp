// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModModel.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModFolderModel.h"

#include <QMessageBox>
#include <algorithm>

namespace ResourceDownload {

ModModel::ModModel(BaseInstance& base_inst, ResourceAPI* api) : ResourceModel(api), m_base_instance(base_inst) {}

/******** Make data requests ********/

ResourceAPI::SearchArgs ModModel::createSearchArguments()
{
    auto profile = static_cast<MinecraftInstance const&>(m_base_instance).getPackProfile();

    Q_ASSERT(profile);
    Q_ASSERT(m_filter);

    std::optional<std::list<Version>> versions{};

    {  // Version filter
        if (!m_filter->versions.empty())
            versions = m_filter->versions;
    }

    auto sort = getCurrentSortingMethodByIndex();

    return { ModPlatform::ResourceType::MOD, m_next_search_offset, m_search_term, sort, profile->getSupportedModLoaders(), versions };
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

    return { pack, versions, profile->getSupportedModLoaders() };
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

bool ModModel::isPackInstalled(ModPlatform::IndexedPack::Ptr pack) const
{
    auto allMods = static_cast<MinecraftInstance&>(m_base_instance).loaderModList()->allMods();
    return std::any_of(allMods.cbegin(), allMods.cend(), [pack](Mod* mod) {
        if (auto meta = mod->metadata(); meta)
            return meta->provider == pack->provider && meta->project_id == pack->addonId;
        return false;
    });
}

}  // namespace ResourceDownload
