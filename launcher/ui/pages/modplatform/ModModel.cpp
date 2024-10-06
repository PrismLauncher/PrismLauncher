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
    std::optional<QStringList> categories{};
    auto loaders = profile->getSupportedModLoaders();

    // Version filter
    if (!m_filter->versions.empty())
        versions = m_filter->versions;
    if (m_filter->loaders)
        loaders = m_filter->loaders;
    if (!m_filter->categoryIds.empty())
        categories = m_filter->categoryIds;
    auto side = m_filter->side;

    auto sort = getCurrentSortingMethodByIndex();

    return { ModPlatform::ResourceType::MOD, m_next_search_offset, m_search_term, sort, loaders, versions, side, categories };
}

ResourceAPI::VersionSearchArgs ModModel::createVersionsArguments(QModelIndex& entry)
{
    auto& pack = *m_packs[entry.row()];
    auto profile = static_cast<MinecraftInstance const&>(m_base_instance).getPackProfile();

    Q_ASSERT(profile);
    Q_ASSERT(m_filter);

    std::optional<std::list<Version>> versions{};
    auto loaders = profile->getSupportedModLoaders();
    if (!m_filter->versions.empty())
        versions = m_filter->versions;
    if (m_filter->loaders)
        loaders = m_filter->loaders;

    return { pack, versions, loaders };
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

QVariant ModModel::getInstalledPackVersion(ModPlatform::IndexedPack::Ptr pack) const
{
    auto allMods = static_cast<MinecraftInstance&>(m_base_instance).loaderModList()->allMods();
    for (auto mod : allMods) {
        if (auto meta = mod->metadata(); meta && meta->provider == pack->provider && meta->project_id == pack->addonId) {
            return meta->version();
        }
    }
    return {};
}

bool checkSide(QString filter, QString value)
{
    return filter.isEmpty() || value.isEmpty() || filter == "both" || value == "both" || filter == value;
}

bool ModModel::checkFilters(ModPlatform::IndexedPack::Ptr pack)
{
    if (!m_filter)
        return true;
    return !(m_filter->hideInstalled && isPackInstalled(pack)) && checkSide(m_filter->side, pack->side);
}

bool ModModel::checkVersionFilters(const ModPlatform::IndexedVersion& v)
{
    if (!m_filter)
        return true;
    auto loaders = static_cast<MinecraftInstance&>(m_base_instance).getPackProfile()->getSupportedModLoaders();
    if (m_filter->loaders)
        loaders = m_filter->loaders;
    return (!optedOut(v) &&                                                         // is opted out(aka curseforge download link)
            (!loaders.has_value() || !v.loaders || loaders.value() & v.loaders) &&  // loaders
            checkSide(m_filter->side, v.side) &&                                    // side
            (m_filter->releases.empty() ||                                          // releases
             std::find(m_filter->releases.cbegin(), m_filter->releases.cend(), v.version_type) != m_filter->releases.cend()) &&
            m_filter->checkMcVersions(v.mcVersion));  // mcVersions
}

}  // namespace ResourceDownload
