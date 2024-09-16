// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "TexturePackModel.h"

#include "Application.h"

#include "meta/Index.h"
#include "meta/Version.h"

static std::list<Version> s_availableVersions = {};

namespace ResourceDownload {
TexturePackResourceModel::TexturePackResourceModel(BaseInstance const& inst, ResourceAPI* api)
    : ResourcePackResourceModel(inst, api), m_version_list(APPLICATION->metadataIndex()->get("net.minecraft"))
{
    if (!m_version_list->isLoaded()) {
        qDebug() << "Loading version list...";
        m_task = m_version_list->getLoadTask();
        if (!m_task->isRunning())
            m_task->start();
    }
}

void waitOnVersionListLoad(Meta::VersionList::Ptr version_list)
{
    QEventLoop load_version_list_loop;

    QTimer time_limit_for_list_load;
    time_limit_for_list_load.setTimerType(Qt::TimerType::CoarseTimer);
    time_limit_for_list_load.setSingleShot(true);
    time_limit_for_list_load.callOnTimeout(&load_version_list_loop, &QEventLoop::quit);
    time_limit_for_list_load.start(4000);

    auto task = version_list->getLoadTask();
    QObject::connect(task.get(), &Task::finished, &load_version_list_loop, &QEventLoop::quit);
    if (!task->isRunning())
        task->start();
    load_version_list_loop.exec();
    if (time_limit_for_list_load.isActive())
        time_limit_for_list_load.stop();
}

ResourceAPI::SearchArgs TexturePackResourceModel::createSearchArguments()
{
    if (s_availableVersions.empty())
        waitOnVersionListLoad(m_version_list);

    auto args = ResourcePackResourceModel::createSearchArguments();

    if (!m_version_list->isLoaded()) {
        qCritical() << "The version list could not be loaded. Falling back to showing all entries.";
        return args;
    }

    if (s_availableVersions.empty()) {
        for (auto&& version : m_version_list->versions()) {
            // FIXME: This duplicates the logic in meta for the 'texturepacks' trait. However, we don't have access to that
            //        information from the index file alone. Also, downloading every version's file isn't a very good idea.
            if (auto ver = version->toComparableVersion(); ver <= maximumTexturePackVersion())
                s_availableVersions.push_back(ver);
        }
    }

    Q_ASSERT(!s_availableVersions.empty());

    args.versions = s_availableVersions;

    return args;
}

ResourceAPI::VersionSearchArgs TexturePackResourceModel::createVersionsArguments(QModelIndex& entry)
{
    auto args = ResourcePackResourceModel::createVersionsArguments(entry);
    if (!m_version_list->isLoaded()) {
        qCritical() << "The version list could not be loaded. Falling back to showing all entries.";
        return args;
    }

    args.mcVersions = s_availableVersions;
    return args;
}

}  // namespace ResourceDownload
