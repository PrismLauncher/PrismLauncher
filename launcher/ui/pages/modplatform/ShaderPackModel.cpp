// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ShaderPackModel.h"

#include <QMessageBox>
#include <utility>

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

namespace ResourceDownload {

ShaderPackResourceModel::ShaderPackResourceModel(const BaseInstance& baseInst,
                                                 const ResourceAPI* api,
                                                 const QString& debugName,
                                                 QString metaEntryBase)
    : ResourceModel(api), m_baseInstance(baseInst), m_debugName(debugName + " (Model)"), m_metaEntryBase(std::move(metaEntryBase))
{}

/******** Make data requests ********/

ResourceAPI::SearchArgs ShaderPackResourceModel::createSearchArguments()
{
    auto sort = getCurrentSortingMethodByIndex();
    return {
        .type = ModPlatform::ResourceType::ShaderPack,
        .offset = m_nextSearchOffset,
        .search = m_searchTerm,
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

    std::optional<std::vector<Version>> versions{};
    auto* profile = static_cast<const MinecraftInstance&>(m_baseInstance).getPackProfile();
    if (auto mcVersion = profile->getComponentVersion("net.minecraft"); !mcVersion.isEmpty()) {
        versions = std::vector<Version>{ Version(mcVersion) };
    }

    return { .pack = pack, .mcVersions = versions, .loaders = {}, .resourceType = ModPlatform::ResourceType::ShaderPack };
}

ResourceAPI::ProjectInfoArgs ShaderPackResourceModel::createInfoArguments(const QModelIndex& entry)
{
    auto pack = m_packs[entry.row()];
    return { .pack = pack };
}

bool ShaderPackResourceModel::checkVersionFilters(const ModPlatform::IndexedVersion& v)
{
    if (optedOut(v)) {
        return false;
    }

    auto* profile = static_cast<const MinecraftInstance&>(m_baseInstance).getPackProfile();
    auto mcVersion = profile->getComponentVersion("net.minecraft");

    // Fall back to accepting the version if either side has no version info to compare against.
    return mcVersion.isEmpty() || v.mcVersion.isEmpty() || v.mcVersion.contains(mcVersion);
}

void ShaderPackResourceModel::searchWithTerm(const QString& term, unsigned int sort)
{
    if (m_searchTerm == term && m_searchTerm.isNull() == term.isNull() && m_currentSortIndex == sort) {
        return;
    }

    setSearchTerm(term);
    m_currentSortIndex = sort;

    refresh();
}

}  // namespace ResourceDownload
