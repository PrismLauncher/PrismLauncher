// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "TexturePackModel.h"

#include <utility>

#include "Application.h"

#include "meta/Index.h"
#include "meta/Version.h"
#include "tasks/Task.h"

static std::vector<Version> s_availableVersions = {};

namespace {
Version maximumTexturePackVersion()
{
    return { "1.6" };
}

void loadVersions(const Meta::VersionList::Ptr& versionList)
{
    s_availableVersions.clear();
    for (auto&& version : versionList->versions()) {
        // FIXME: This duplicates the logic in meta for the 'texturepacks' trait. However, we don't have access to that
        //        information from the index file alone. Also, downloading every version's file isn't a very good idea.
        if (auto ver = version->toComparableVersion(); ver <= maximumTexturePackVersion()) {
            s_availableVersions.push_back(ver);
        }
    }
}
}  // namespace

namespace ResourceDownload {
TexturePackResourceModel::TexturePackResourceModel(ResourceFolderModel* inst,
                                                   const ResourceAPI* api,
                                                   const QString& debugName,
                                                   QString metaEntryBase)
    : ResourcePackResourceModel(inst, api, debugName, std::move(metaEntryBase))
    , m_versionList(APPLICATION->metadataIndex()->get("net.minecraft"))
{
    if (!m_versionList->isLoaded()) {
        qDebug() << "Loading version list...";
        m_task = m_versionList->getLoadTask();
        connect(m_task.get(), &Task::finished, this, [this] { loadVersions(m_versionList); });
        if (!m_task->isRunning()) {
            m_task->start();
        }
    }
}

ResourceAPI::SearchArgs TexturePackResourceModel::createSearchArguments()
{
    auto args = ResourcePackResourceModel::createSearchArguments();

    args.versions = { maximumTexturePackVersion() };

    if (!m_versionList->isLoaded()) {
        qCritical() << "The version list could not be loaded. Falling back to showing all entries.";
        return args;
    }

    if (s_availableVersions.empty()) {
        loadVersions(m_versionList);
    }

    Q_ASSERT(!s_availableVersions.empty());

    args.versions = s_availableVersions;

    return args;
}

ResourceAPI::VersionSearchArgs TexturePackResourceModel::createVersionsArguments(const QModelIndex& entry)
{
    auto args = ResourcePackResourceModel::createVersionsArguments(entry);
    args.resourceType = ModPlatform::ResourceType::TexturePack;
    if (!m_versionList->isLoaded()) {
        qCritical() << "The version list could not be loaded. Falling back to showing all entries.";
        return args;
    }

    args.mcVersions = s_availableVersions;
    return args;
}

}  // namespace ResourceDownload
