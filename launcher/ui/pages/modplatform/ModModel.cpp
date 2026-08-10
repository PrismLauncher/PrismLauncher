// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ModModel.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/ResourceType.h"
#include "ui/pages/modplatform/ResourceModel.h"

#include <QMessageBox>
#include <QModelIndex>
#include <QString>
#include <algorithm>
#include <utility>

namespace ResourceDownload {

ModModel::ModModel(BaseInstance& baseInst, const ResourceAPI* api, const QString& debugName, QString metaEntryBase)
    : ResourceModel(api), m_baseInstance(baseInst), m_debugName(debugName + " (Model)"), m_metaEntryBase(std::move(metaEntryBase))
{}

/******** Make data requests ********/

ResourceAPI::SearchArgs ModModel::createSearchArguments()
{
    auto* profile = static_cast<const MinecraftInstance&>(m_baseInstance).getPackProfile();

    Q_ASSERT(profile);
    Q_ASSERT(m_filter);

    std::optional<std::vector<Version>> versions{};
    std::optional<QStringList> categories{};
    auto loaders = profile->getSupportedModLoaders();

    // Version filter
    if (!m_filter->versions.empty()) {
        versions = m_filter->versions;
    }
    if (m_filter->loaders != 0U) {
        loaders = m_filter->loaders;
    }
    if (!m_filter->categoryIds.empty()) {
        categories = m_filter->categoryIds;
    }
    auto side = m_filter->side;

    auto sort = getCurrentSortingMethodByIndex();

    return { .type = ModPlatform::ResourceType::Mod,
             .offset = m_next_search_offset,
             .search = m_search_term,
             .sorting = sort,
             .loaders = loaders,
             .versions = versions,
             .side = side,
             .categoryIds = categories,
             .openSource = m_filter->openSource };
}

ResourceAPI::VersionSearchArgs ModModel::createVersionsArguments(const QModelIndex& index)
{
    auto pack = m_packs[index.row()];
    auto* profile = static_cast<const MinecraftInstance&>(m_baseInstance).getPackProfile();

    Q_ASSERT(profile);
    Q_ASSERT(m_filter);

    std::optional<std::vector<Version>> versions{};
    auto loaders = profile->getSupportedModLoaders();
    if (!m_filter->versions.empty()) {
        versions = m_filter->versions;
    }
    if (m_filter->loaders != 0U) {
        loaders = m_filter->loaders;
    }

    return { .pack = pack, .mcVersions = versions, .loaders = loaders, .resourceType = ModPlatform::ResourceType::Mod };
}

QString ModModel::createInfoArguments(const QModelIndex& index)
{
    auto pack = m_packs[index.row()];
    return pack->addonId.toString();
}

void ModModel::searchWithTerm(const QString& term, unsigned int sort, bool filterChanged)
{
    if (m_search_term == term && m_search_term.isNull() == term.isNull() && m_current_sort_index == sort && !filterChanged) {
        return;
    }

    setSearchTerm(term);
    m_current_sort_index = sort;

    refresh();
}

bool ModModel::isPackInstalled(ModPlatform::IndexedPack::Ptr pack) const
{
    auto allMods = static_cast<MinecraftInstance&>(m_baseInstance).loaderModList()->allMods();
    return std::ranges::any_of(allMods, [pack](Mod* mod) {
        if (auto meta = mod->metadata(); meta) {
            return meta->provider == pack->provider && meta->project_id == pack->addonId;
        }
        return false;
    });
}

QVariant ModModel::getInstalledPackVersion(ModPlatform::IndexedPack::Ptr pack) const
{
    auto allMods = static_cast<MinecraftInstance&>(m_baseInstance).loaderModList()->allMods();
    for (auto* mod : allMods) {
        if (auto meta = mod->metadata(); meta && meta->provider == pack->provider && meta->project_id == pack->addonId) {
            return meta->version();
        }
    }
    return {};
}

namespace {

bool checkSide(ModPlatform::SideType filter, ModPlatform::SideType value)
{
    return (filter != ModPlatform::SideType::ClientSide && filter != ModPlatform::SideType::ServerSide) ||
           (value != ModPlatform::SideType::ClientSide && value != ModPlatform::SideType::ServerSide) || filter == value;
}
}  // namespace

bool ModModel::checkFilters(ModPlatform::IndexedPack::Ptr pack)
{
    if (!m_filter) {
        return true;
    }
    return !(m_filter->hideInstalled && isPackInstalled(pack)) && checkSide(m_filter->side, pack->side);
}

bool ModModel::checkVersionFilters(const ModPlatform::IndexedVersion& v)
{
    if (!m_filter) {
        return true;
    }
    auto loaders = static_cast<MinecraftInstance&>(m_baseInstance).getPackProfile()->getSupportedModLoaders();
    if (m_filter->loaders != 0U) {
        loaders = m_filter->loaders;
    }
    return (!optedOut(v) &&                                                                   // is opted out(aka curseforge download link)
            (!loaders.has_value() || !v.loaders || ((loaders.value() & v.loaders) != 0U)) &&  // loaders
            checkSide(m_filter->side, v.side) &&                                              // side
            (m_filter->releases.empty() ||                                                    // releases
             std::find(m_filter->releases.cbegin(), m_filter->releases.cend(), v.version_type) != m_filter->releases.cend()) &&
            m_filter->checkMcVersions(v.mcVersion));  // mcVersions
}

}  // namespace ResourceDownload
