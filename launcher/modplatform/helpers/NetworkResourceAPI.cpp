// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "NetworkResourceAPI.h"
#include <memory>

#include "Application.h"
#include "net/NetJob.h"

#include "modplatform/ModIndex.h"

#include "net/ApiDownload.h"

Task::Ptr NetworkResourceAPI::getProject(QString addonId, std::shared_ptr<QByteArray> response) const
{
    auto project_url_optional = getInfoURL(addonId);
    if (!project_url_optional.has_value())
        return nullptr;

    auto project_url = project_url_optional.value();

    auto netJob = makeShared<NetJob>(QString("%1::GetProject").arg(addonId), APPLICATION->network());

    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(project_url), response));

    return netJob;
}

Task::Ptr NetworkResourceAPI::getDependencyVersion(DependencySearchArgs&& args, DependencySearchCallbacks&& callbacks) const
{
    auto versions_url_optional = getDependencyURL(args);
    if (!versions_url_optional.has_value())
        return nullptr;

    auto versions_url = versions_url_optional.value();

    auto netJob = makeShared<NetJob>(QString("%1::Dependency").arg(args.dependency.addonId.toString()), APPLICATION->network());
    auto response = std::make_shared<QByteArray>();

    netJob->addNetAction(Net::ApiDownload::makeByteArray(versions_url, response));

    QObject::connect(netJob.get(), &NetJob::succeeded, [response, callbacks, args] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for getting versions at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        callbacks.on_succeed(doc, args.dependency);
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
