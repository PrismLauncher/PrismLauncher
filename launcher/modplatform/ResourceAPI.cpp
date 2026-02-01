#include "modplatform/ResourceAPI.h"

#include "Application.h"
#include "Json.h"
#include "net/NetJob.h"

#include "modplatform/ModIndex.h"

#include "net/ApiDownload.h"

Task::Ptr ResourceAPI::searchProjects(SearchArgs&& args, Callback<QList<ModPlatform::IndexedPack::Ptr>>&& callbacks) const
{
    auto search_url_optional = getSearchURL(args);
    if (!search_url_optional.has_value()) {
        callbacks.on_fail("Failed to create search URL", -1);
        return nullptr;
    }

    auto search_url = search_url_optional.value();

    auto response = std::make_shared<QByteArray>();
    auto netJob = makeShared<NetJob>(QString("%1::Search").arg(debugName()), APPLICATION->network());

    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(search_url), response.get()));

    QObject::connect(netJob.get(), &NetJob::succeeded, [this, response, callbacks] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from" << debugName() << "at" << parse_error.offset
                       << "reason:" << parse_error.errorString();
            qWarning() << *response;

            callbacks.on_fail(parse_error.errorString(), -1);

            return;
        }

        QList<ModPlatform::IndexedPack::Ptr> newList;
        auto packs = documentToArray(doc);

        for (auto packRaw : packs) {
            auto packObj = packRaw.toObject();

            ModPlatform::IndexedPack::Ptr pack = std::make_shared<ModPlatform::IndexedPack>();
            try {
                loadIndexedPack(*pack, packObj);
                newList << pack;
            } catch (const JSONValidationError& e) {
                qWarning().nospace() << "Error while loading resource from " << debugName() << ": " << e.cause();
                continue;
            }
        }

        callbacks.on_succeed(newList);
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
    QObject::connect(netJob.get(), &NetJob::aborted, [callbacks] {
        if (callbacks.on_abort != nullptr)
            callbacks.on_abort();
    });

    return netJob;
}

Task::Ptr ResourceAPI::getProjectVersions(VersionSearchArgs&& args, Callback<QVector<ModPlatform::IndexedVersion>>&& callbacks) const
{
    auto versions_url_optional = getVersionsURL(args);
    if (!versions_url_optional.has_value())
        return nullptr;

    auto versions_url = versions_url_optional.value();

    auto netJob = makeShared<NetJob>(QString("%1::Versions").arg(args.pack->name), APPLICATION->network());
    auto response = std::make_shared<QByteArray>();

    netJob->addNetAction(Net::ApiDownload::makeByteArray(versions_url, response.get()));

    QObject::connect(netJob.get(), &NetJob::succeeded, [this, response, callbacks, args] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for getting versions at" << parse_error.offset
                       << "reason:" << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        QVector<ModPlatform::IndexedVersion> unsortedVersions;
        try {
            auto arr = doc.isObject() ? doc.object()["data"].toArray() : doc.array();

            for (auto versionIter : arr) {
                auto obj = versionIter.toObject();

                auto file = loadIndexedPackVersion(obj, args.resourceType);
                if (!file.addonId.isValid())
                    file.addonId = args.pack->addonId;

                if (file.fileId.isValid() && !file.downloadUrl.isEmpty())  // Heuristic to check if the returned value is valid
                    unsortedVersions.append(file);
            }

            auto orderSortPredicate = [](const ModPlatform::IndexedVersion& a, const ModPlatform::IndexedVersion& b) -> bool {
                // dates are in RFC 3339 format
                return a.date > b.date;
            };
            std::sort(unsortedVersions.begin(), unsortedVersions.end(), orderSortPredicate);
        } catch (const JSONValidationError& e) {
            qDebug() << doc;
            qWarning() << "Error while reading" << debugName() << "resource version:" << e.cause();
        }

        callbacks.on_succeed(unsortedVersions);
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
    QObject::connect(netJob.get(), &NetJob::aborted, [callbacks] {
        if (callbacks.on_abort != nullptr)
            callbacks.on_abort();
    });

    return netJob;
}

Task::Ptr ResourceAPI::getProjectInfo(ProjectInfoArgs&& args, Callback<ModPlatform::IndexedPack::Ptr>&& callbacks) const
{
    auto response = std::make_shared<QByteArray>();
    auto job = getProject(args.pack->addonId.toString(), response.get());

    QObject::connect(job.get(), &NetJob::succeeded, [this, response, callbacks, args] {
        auto pack = args.pack;
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for mod info at" << parse_error.offset
                       << "reason:" << parse_error.errorString();
            qWarning() << *response;
            return;
        }
        try {
            auto obj = Json::requireObject(doc);
            if (obj.contains("data"))
                obj = Json::requireObject(obj, "data");
            loadIndexedPack(*pack, obj);
            loadExtraPackInfo(*pack, obj);
        } catch (const JSONValidationError& e) {
            qDebug() << doc;
            qWarning() << "Error while reading" << debugName() << "resource info:" << e.cause();
        }
        callbacks.on_succeed(pack);
    });
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
    QObject::connect(job.get(), &NetJob::aborted, [callbacks] {
        if (callbacks.on_abort != nullptr)
            callbacks.on_abort();
    });
    return job;
}

Task::Ptr ResourceAPI::getDependencyVersion(DependencySearchArgs&& args, Callback<ModPlatform::IndexedVersion>&& callbacks) const
{
    auto versions_url_optional = getDependencyURL(args);
    if (!versions_url_optional.has_value())
        return nullptr;

    auto versions_url = versions_url_optional.value();

    auto netJob = makeShared<NetJob>(QString("%1::Dependency").arg(args.dependency.addonId.toString()), APPLICATION->network());
    auto response = std::make_shared<QByteArray>();

    netJob->addNetAction(Net::ApiDownload::makeByteArray(versions_url, response.get()));

    QObject::connect(netJob.get(), &NetJob::succeeded, [this, response, callbacks, args] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for getting dependency version at" << parse_error.offset
                       << "reason:" << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        QJsonArray arr;
        if (args.dependency.version.length() != 0 && doc.isObject()) {
            arr.append(doc.object());
        } else {
            arr = doc.isObject() ? doc.object()["data"].toArray() : doc.array();
        }

        QVector<ModPlatform::IndexedVersion> versions;
        for (auto versionIter : arr) {
            auto obj = versionIter.toObject();

            auto file = loadIndexedPackVersion(obj, ModPlatform::ResourceType::Mod);
            if (!file.addonId.isValid())
                file.addonId = args.dependency.addonId;

            if (file.fileId.isValid() &&
                (!file.loaders || args.loader & file.loaders))  // Heuristic to check if the returned value is valid
                versions.append(file);
        }

        auto orderSortPredicate = [](const ModPlatform::IndexedVersion& a, const ModPlatform::IndexedVersion& b) -> bool {
            // dates are in RFC 3339 format
            return a.date > b.date;
        };
        std::sort(versions.begin(), versions.end(), orderSortPredicate);
        auto bestMatch = versions.size() != 0 ? versions.front() : ModPlatform::IndexedVersion();
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

QString ResourceAPI::getGameVersionsString(std::vector<Version> mcVersions) const
{
    QString s;
    for (auto& ver : mcVersions) {
        s += QString("\"%1\",").arg(mapMCVersionToModrinth(ver));
    }
    s.remove(s.length() - 1, 1);  // remove last comma
    return s;
}

QString ResourceAPI::mapMCVersionToModrinth(Version v) const
{
    static const QString preString = " Pre-Release ";
    auto verStr = v.toString();

    if (verStr.contains(preString)) {
        verStr.replace(preString, "-pre");
    }
    verStr.replace(" ", "-");
    return verStr;
}

Task::Ptr ResourceAPI::getProject(QString addonId, QByteArray* response) const
{
    auto project_url_optional = getInfoURL(addonId);
    if (!project_url_optional.has_value())
        return nullptr;

    auto project_url = project_url_optional.value();

    auto netJob = makeShared<NetJob>(QString("%1::GetProject").arg(addonId), APPLICATION->network());

    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(project_url), response));

    return netJob;
}
