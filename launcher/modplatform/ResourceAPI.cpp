#include "modplatform/ResourceAPI.h"

#include "Application.h"
#include "net/NetJob.h"

#include "modplatform/ModIndex.h"

#include "net/RPCSink.h"

std::pair<NetJob::Ptr, ModPlatform::IndexedPack*> ResourceAPI::getProjectTask(const QString& addonId, bool loadExtra, bool askRetry) const
{
    auto netJob = makeShared<NetJob>(QString("%1::GetProject").arg(addonId), APPLICATION->network());
    netJob->setAskRetry(askRetry);

    auto spec = getProject(addonId);
    auto [action, response] = Net::RPC::make<ModPlatform::IndexedPack>(spec);
    netJob->addNetAction(action);
    netJob->setMaxConcurrent(1);  // force extra to load in sync

    response->addonId = addonId;
    if (loadExtra) {
        auto extraSpec = getProjectExtra(*response);
        if (extraSpec.has_value()) {
            auto [extraAction, _] = Net::RPC::make<bool>(extraSpec.value());
            netJob->addNetAction(extraAction);
        }
    }

    return { netJob, response };
}

std::pair<NetJob::Ptr, QList<ModPlatform::IndexedPack>*> ResourceAPI::searchProjectsTask(const SearchArgs& args) const
{
    auto spec = searchProjects(args);

    auto netJob = makeShared<NetJob>(QString("%1::Search").arg(debugName()), APPLICATION->network());

    auto [action, response] = Net::RPC::make<QList<ModPlatform::IndexedPack>>(spec);
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<NetJob::Ptr, QList<ModPlatform::IndexedPack>*> ResourceAPI::getProjectsTask(const QStringList& addonIds) const
{
    auto spec = getProjects(addonIds);

    auto netJob = makeShared<NetJob>(QString("%1::List").arg(debugName()), APPLICATION->network());

    auto [action, response] = Net::RPC::make<QList<ModPlatform::IndexedPack>>(spec);
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<NetJob::Ptr, QList<ModPlatform::Category>*> ResourceAPI::getCategoriesTask(ModPlatform::ResourceType type) const
{
    auto spec = getCategories(type);

    auto netJob = makeShared<NetJob>(QString("%1::Categories").arg(debugName()), APPLICATION->network());

    auto [action, response] = Net::RPC::make<QList<ModPlatform::Category>>(spec);
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<NetJob::Ptr, QList<ModPlatform::IndexedVersion>*> ResourceAPI::getVersionsTask(const VersionSearchArgs& args) const
{
    auto spec = getVersions(args);

    auto netJob = makeShared<NetJob>(QString("%1::Versions").arg(debugName()), APPLICATION->network());

    auto [action, response] = Net::RPC::make<QList<ModPlatform::IndexedVersion>>(spec);
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<NetJob::Ptr, QList<ModPlatform::IndexedVersion>*> ResourceAPI::getVersionsTask(const QStringList& versionIds) const
{
    auto spec = getVersions(versionIds);

    auto netJob = makeShared<NetJob>(QString("%1::Versions").arg(debugName()), APPLICATION->network());

    auto [action, response] = Net::RPC::make<QList<ModPlatform::IndexedVersion>>(spec);
    netJob->addNetAction(action);

    return { netJob, response };
}

std::pair<NetJob::Ptr, ModPlatform::IndexedVersion*> ResourceAPI::getVersionTask(const QString& id, const QString& versionId) const
{
    auto spec = getVersion(id, versionId);

    auto netJob = makeShared<NetJob>(QString("%1::Version").arg(debugName()), APPLICATION->network());

    auto [action, response] = Net::RPC::make<ModPlatform::IndexedVersion>(spec);
    netJob->addNetAction(action);

    return { netJob, response };
}
