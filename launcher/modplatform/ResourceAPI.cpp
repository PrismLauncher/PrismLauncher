#include "modplatform/ResourceAPI.h"
#include <memory>

#include "Application.h"
#include "api/Api.h"
#include "api/structures/Arguments.h"
#include "net/NetJob.h"

#include "api/structures/Project.h"

Task::Ptr ResourceAPI::searchProjects(API::SearchArgs&& args, API::Callback<QList<Platform::Project::Ptr>>&& callbacks) const
{
    std::shared_ptr<QList<Platform::Project::Ptr>> newList = std::make_shared<QList<Platform::Project::Ptr>>();
    auto job = API::ProviderAPI::get(provider())->makeSearchRequest(args, newList);
    if (!job) {
        callbacks.on_fail("Failed to create search URL", -1);
        return nullptr;
    }

    auto netJob = makeShared<NetJob>(QString("%1::Search").arg(debugName()), APPLICATION->network());

    netJob->addNetAction(job);

    QObject::connect(netJob.get(), &NetJob::succeeded, [newList, callbacks] { callbacks.on_succeed(*newList); });

    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = netJob.toWeakRef();
    QObject::connect(netJob.get(), &NetJob::failed, [weak, callbacks](const QString& reason) {
        int network_error_code = -1;
        if (auto netJob = weak.lock()) {
            if (auto* failed_action = netJob->getFailedActions().at(0); failed_action)
                network_error_code = failed_action->replyStatusCode();
        }
        callbacks.on_fail(reason, network_error_code);
    });
    QObject::connect(netJob.get(), &NetJob::aborted, [callbacks] { callbacks.on_abort(); });

    return netJob;
}

Task::Ptr ResourceAPI::getProjectVersions(API::VersionSearchArgs&& args, API::Callback<QVector<Platform::Version>>&& callbacks) const
{
    auto response = std::make_shared<API::VersionSearchResponse>();
    response->projectId = args.pack.projectId;
    response->resourceType = args.resourceType;
    auto versionJob = API::ProviderAPI::get(provider())->makeGetVersionsRequest(args, response);

    auto netJob = makeShared<NetJob>(QString("%1::Versions").arg(args.pack.name), APPLICATION->network());

    netJob->addNetAction(versionJob);

    QObject::connect(netJob.get(), &NetJob::succeeded, [response, callbacks] { callbacks.on_succeed(response->versions); });

    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = netJob.toWeakRef();
    QObject::connect(netJob.get(), &NetJob::failed, [weak, callbacks](const QString& reason) {
        int network_error_code = -1;
        if (auto netJob = weak.lock()) {
            if (auto* failed_action = netJob->getFailedActions().at(0); failed_action)
                network_error_code = failed_action->replyStatusCode();
        }
        callbacks.on_fail(reason, network_error_code);
    });
    QObject::connect(netJob.get(), &NetJob::aborted, [callbacks] { callbacks.on_abort(); });

    return netJob;
}

Task::Ptr ResourceAPI::getProjectInfo(API::ProjectInfoArgs&& args, API::Callback<Platform::Project>&& callbacks) const
{
    auto response = args.pack;
    auto projectId = args.pack->projectId.toString();
    auto job = makeShared<NetJob>(QString("%1::GetProject").arg(projectId), APPLICATION->network(), 1);
    auto projectRequest = API::ProviderAPI::get(provider())->makeGetProjectRequest(projectId, response);
    auto descriptionRequest = API::ProviderAPI::get(provider())->makeGetDescriptionRequest(projectId, response);
    job->addNetAction(projectRequest);
    if (descriptionRequest) {
        job->addNetAction(descriptionRequest);
    }

    QObject ::connect(job.get(), &NetJob::succeeded, [response, callbacks] { callbacks.on_succeed(*response); });
    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = job.toWeakRef();
    QObject::connect(job.get(), &NetJob::failed, [weak, callbacks](const QString& reason) {
        int network_error_code = -1;
        if (auto job = weak.lock()) {
            if (auto netJob = qSharedPointerDynamicCast<NetJob>(job)) {
                if (auto* failed_action = netJob->getFailedActions().at(0); failed_action) {
                    network_error_code = failed_action->replyStatusCode();
                }
            }
        }
        callbacks.on_fail(reason, network_error_code);
    });
    QObject::connect(job.get(), &NetJob::aborted, [callbacks] { callbacks.on_abort(); });
    return job;
}

Task::Ptr ResourceAPI::getDependencyVersion(API::DependencySearchArgs&& args, API::Callback<Platform::Version>&& callbacks) const
{
    auto response = std::make_shared<API::VersionSearchResponse>();
    response->projectId = args.dependency.projectId;
    response->resourceType = Platform::ResourceType::Mod;
    auto versionJob = API::ProviderAPI::get(provider())->makeGetDependencyRequest(args, response);

    auto netJob = makeShared<NetJob>(QString("%1::Dependency").arg(args.dependency.projectId.toString()), APPLICATION->network());

    netJob->addNetAction(versionJob);

    QObject::connect(netJob.get(), &NetJob::succeeded, [response, callbacks] {
        auto bestMatch = response->versions.size() != 0 ? response->versions.front() : Platform::Version();
        callbacks.on_succeed(bestMatch);
    });

    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = netJob.toWeakRef();
    QObject::connect(netJob.get(), &NetJob::failed, [weak, callbacks](const QString& reason) {
        int network_error_code = -1;
        if (auto netJob = weak.lock()) {
            if (auto* failed_action = netJob->getFailedActions().at(0); failed_action)
                network_error_code = failed_action->replyStatusCode();
        }
        callbacks.on_fail(reason, network_error_code);
    });
    return netJob;
}

QString ResourceAPI::getModFileChangelog(QVariant modId, QVariant fileId)
{
    QEventLoop lock;

    auto netJob = makeShared<NetJob>(QString("%1::FileChangelog").arg(debugName()), APPLICATION->network());
    auto response = std::make_shared<QString>();
    auto task = API::ProviderAPI::get(provider())->makeGetFileChangelogRequest({ modId, fileId }, response);
    if (task) {
        netJob->addNetAction(task);

        QObject::connect(netJob.get(), &NetJob::finished, [&lock] { lock.quit(); });

        netJob->start();
        lock.exec();
    }

    return *response;
}
