#include "modplatform/ResourceAPI.h"

#include <algorithm>

#include "Application.h"
#include "Json.h"
#include "net/NetJob.h"

#include "modplatform/ModIndex.h"

#include "net/ApiRequest.h"

Task::Ptr ResourceAPI::searchProjects(const SearchArgs& args, const Callback<QList<ModPlatform::IndexedPack::Ptr>>& callbacks) const
{
    auto searchUrlOptional = getSearchURL(args);
    if (!searchUrlOptional.has_value()) {
        callbacks.onFail("Failed to create search URL", -1);
        return nullptr;
    }

    const auto& searchUrl = searchUrlOptional.value();

    auto netJob = makeShared<NetJob>(QString("%1::Search").arg(debugName()), APPLICATION->network());

    auto [action, response] = Net::ApiRequest::makeByteArray(QUrl(searchUrl));
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::succeeded, netJob.get(), [this, response, callbacks] {
        auto doc = Json::requireDocument(*response, "ResourceAPI");
        if (!doc) {
            qWarning() << "Error while parsing JSON response from" << debugName() << ":" << doc.error();
            qWarning() << *response;

            callbacks.onFail(doc.error(), -1);

            return;
        }

        QList<ModPlatform::IndexedPack::Ptr> newList;
        auto packs = documentToArray(doc.value());

        for (auto packRaw : packs) {
            auto packObj = packRaw.toObject();

            ModPlatform::IndexedPack::Ptr pack = std::make_shared<ModPlatform::IndexedPack>();
            auto loadRes = loadIndexedPack(*pack, packObj);
            if (!loadRes) {
                qWarning().nospace() << "Error while loading resource from " << debugName() << ": " << loadRes.error();
                continue;
            }
            newList << pack;
        }

        callbacks.onSucceed(newList);
    });

    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = netJob.toWeakRef();
    QObject::connect(netJob.get(), &NetJob::failed, netJob.get(), [weak, callbacks](const QString& reason) {
        int networkErrorCode = -1;
        if (auto netJob = weak.lock()) {
            if (auto* failedAction = netJob->getFailedActions().at(0); failedAction) {
                networkErrorCode = failedAction->replyStatusCode();
            }
        }
        callbacks.onFail(reason, networkErrorCode);
    });
    QObject::connect(netJob.get(), &NetJob::aborted, netJob.get(), [callbacks] {
        if (callbacks.onAbort != nullptr) {
            callbacks.onAbort();
        }
    });

    return netJob;
}

Task::Ptr ResourceAPI::getProjectVersions(const VersionSearchArgs& args,
                                          const Callback<QVector<ModPlatform::IndexedVersion>>& callbacks) const
{
    auto versionsUrlOptional = getVersionsURL(args);
    if (!versionsUrlOptional.has_value()) {
        return nullptr;
    }

    const auto& versionsUrl = versionsUrlOptional.value();

    auto netJob = makeShared<NetJob>(QString("%1::Versions").arg(args.pack->name), APPLICATION->network());

    auto [action, response] = Net::ApiRequest::makeByteArray(versionsUrl);
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::succeeded, netJob.get(), [this, response, callbacks, args] {
        auto doc = Json::requireDocument(*response, "ResourceAPI::getProjectVersions");
        if (!doc) {
            qWarning() << "Error while parsing JSON response for getting versions:" << doc.error();
            qWarning() << *response;
            return;
        }

        QVector<ModPlatform::IndexedVersion> unsortedVersions;
        auto arr = doc.value().isObject() ? doc.value().object()["data"].toArray() : doc.value().array();

        for (auto versionIter : arr) {
            auto obj = versionIter.toObject();

            auto fileRes = loadIndexedPackVersion(obj, args.resourceType);
            if (!fileRes) {
                qWarning() << "Error while reading" << debugName() << "resource version:" << fileRes.error();
                continue;
            }
            auto file = fileRes.value();
            if (!file.addonId.isValid()) {
                file.addonId = args.pack->addonId;
            }

            if (file.fileId.isValid() && !file.downloadUrl.isEmpty()) {  // Heuristic to check if the returned value is valid
                unsortedVersions.append(file);
            }
        }

        auto orderSortPredicate = [](const ModPlatform::IndexedVersion& a, const ModPlatform::IndexedVersion& b) -> bool {
            // dates are in RFC 3339 format
            return a.date > b.date;
        };
        std::ranges::sort(unsortedVersions, orderSortPredicate);

        callbacks.onSucceed(unsortedVersions);
    });

    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = netJob.toWeakRef();
    QObject::connect(netJob.get(), &NetJob::failed, netJob.get(), [weak, callbacks](const QString& reason) {
        int networkErrorCode = -1;
        if (auto netJob = weak.lock()) {
            if (auto* failedAction = netJob->getFailedActions().at(0); failedAction) {
                networkErrorCode = failedAction->replyStatusCode();
            }
        }
        callbacks.onFail(reason, networkErrorCode);
    });
    QObject::connect(netJob.get(), &NetJob::aborted, netJob.get(), [callbacks] {
        if (callbacks.onAbort != nullptr) {
            callbacks.onAbort();
        }
    });

    return netJob;
}

Task::Ptr ResourceAPI::getProjectInfo(const ProjectInfoArgs& args,
                                      const Callback<ModPlatform::IndexedPack::Ptr>& callbacks,
                                      bool askRetry) const
{
    auto [job, response] = getProject(args.pack->addonId.toString(), askRetry);

    QObject::connect(job.get(), &NetJob::succeeded, job.get(), [this, response, callbacks, args] {
        auto pack = args.pack;
        auto parse = [this, &pack, &response]() -> Result<> {
            auto doc = Json::requireDocument(*response).and_then([](const auto& v) { return Json::requireObject(v); });
            TRY(doc)
            auto obj = doc.value();
            if (obj.contains("data")) {
                auto dataObj = Json::requireObject(obj, "data");
                TRY(dataObj)
                obj = dataObj.value();
            }
            auto loadRes = loadIndexedPack(*pack, obj);
            TRY(loadRes)

            return loadExtraPackInfo(*pack, obj);
        };
        if (auto rsp = parse(); !rsp) {
            qWarning() << "Error while reading" << debugName() << "resource info:" << rsp.error();
            return;
        }

        callbacks.onSucceed(pack);
    });
    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = job.toWeakRef();
    QObject::connect(job.get(), &NetJob::failed, job.get(), [weak, callbacks](const QString& reason) {
        int networkErrorCode = -1;
        if (auto job = weak.lock()) {
            if (auto netJob = qSharedPointerDynamicCast<NetJob>(job)) {
                if (auto* failedAction = netJob->getFailedActions().at(0); failedAction) {
                    networkErrorCode = failedAction->replyStatusCode();
                }
            }
        }
        callbacks.onFail(reason, networkErrorCode);
    });
    QObject::connect(job.get(), &NetJob::aborted, job.get(), [callbacks] {
        if (callbacks.onAbort != nullptr) {
            callbacks.onAbort();
        }
    });
    return job;
}

Task::Ptr ResourceAPI::getDependencyVersion(const DependencySearchArgs& args, const Callback<ModPlatform::IndexedVersion>& callbacks) const
{
    auto versionsUrlOptional = getDependencyURL(args);
    if (!versionsUrlOptional.has_value()) {
        return nullptr;
    }

    const auto& versionsUrl = versionsUrlOptional.value();

    auto netJob = makeShared<NetJob>(QString("%1::Dependency").arg(args.dependency.addonId.toString()), APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(versionsUrl);
    netJob->addNetAction(action);

    QObject::connect(netJob.get(), &NetJob::succeeded, netJob.get(), [this, response, callbacks, args] {
        auto doc = Json::requireDocument(*response, "ResourceAPI::getDependencyVersions");
        if (!doc) {
            qWarning() << "Error while parsing JSON response for getting dependency version:" << doc.error();
            qWarning() << *response;
            return;
        }

        QJsonArray arr;
        if (args.dependency.version.length() != 0 && doc.value().isObject()) {
            arr.append(doc.value().object());
        } else {
            arr = doc.value().isObject() ? doc.value().object()["data"].toArray() : doc.value().array();
        }

        QVector<ModPlatform::IndexedVersion> versions;
        for (auto versionIter : arr) {
            auto obj = versionIter.toObject();

            auto fileRes = loadIndexedPackVersion(obj, ModPlatform::ResourceType::Mod);
            if (!fileRes) {
                qWarning() << "Error while reading" << debugName() << "resource version:" << fileRes.error();
                continue;
            }
            auto file = fileRes.value();
            if (!file.addonId.isValid()) {
                file.addonId = args.dependency.addonId;
            }

            if (file.fileId.isValid() &&
                (!file.loaders || args.loader & file.loaders)) {  // Heuristic to check if the returned value is valid
                versions.append(file);
            }
        }

        auto orderSortPredicate = [](const ModPlatform::IndexedVersion& a, const ModPlatform::IndexedVersion& b) -> bool {
            // dates are in RFC 3339 format
            return a.date > b.date;
        };
        std::ranges::sort(versions, orderSortPredicate);
        auto bestMatch = versions.size() != 0 ? versions.front() : ModPlatform::IndexedVersion();
        callbacks.onSucceed(bestMatch);
    });

    // Capture a weak_ptr instead of a shared_ptr to avoid circular dependency issues.
    // This prevents the lambda from extending the lifetime of the shared resource,
    // as it only temporarily locks the resource when needed.
    auto weak = netJob.toWeakRef();
    QObject::connect(netJob.get(), &NetJob::failed, netJob.get(), [weak, callbacks](const QString& reason) {
        int networkErrorCode = -1;
        if (auto netJob = weak.lock()) {
            if (auto* failedAction = netJob->getFailedActions().at(0); failedAction) {
                networkErrorCode = failedAction->replyStatusCode();
            }
        }
        callbacks.onFail(reason, networkErrorCode);
    });
    return netJob;
}

QString ResourceAPI::getGameVersionsString(const std::vector<Version>& mcVersions)
{
    QString s;
    for (const auto& ver : mcVersions) {
        s += QString("\"%1\",").arg(mapMCVersionToModrinth(ver));
    }
    s.remove(s.length() - 1, 1);  // remove last comma
    return s;
}

QString ResourceAPI::mapMCVersionToModrinth(const Version& v)
{
    static const QString s_preString = " Pre-Release ";
    auto verStr = v.toString();

    if (verStr.contains(s_preString)) {
        verStr.replace(s_preString, "-pre");
    }
    verStr.replace(" ", "-");
    return verStr;
}

std::pair<Task::Ptr, QByteArray*> ResourceAPI::getProject(const QString& addonId, bool askRetry) const
{
    auto projectUrlOptional = getInfoURL(addonId);
    if (!projectUrlOptional.has_value()) {
        return { nullptr, nullptr };
    }

    const auto& projectUrl = projectUrlOptional.value();

    auto netJob = makeShared<NetJob>(QString("%1::GetProject").arg(addonId), APPLICATION->network());
    netJob->setAskRetry(askRetry);

    auto [action, response] = Net::ApiRequest::makeByteArray(QUrl(projectUrl));
    netJob->addNetAction(action);

    return { netJob, response };
}
