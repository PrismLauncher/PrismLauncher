// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "NetworkResourceAPI.h"
#include <memory>

#include "Application.h"
#include "net/NetJob.h"

#include "modplatform/ModIndex.h"

#include "net/ApiDownload.h"

TaskV2::Ptr NetworkResourceAPI::searchProjects(SearchArgs&& args, SearchCallbacks&& callbacks) const
{
    auto search_url_optional = getSearchURL(args);
    if (!search_url_optional.has_value()) {
        callbacks.on_fail("Failed to create search URL", -1);
        return nullptr;
    }

    auto search_url = search_url_optional.value();

    auto response = std::make_shared<QByteArray>();
    auto netJob = makeShared<NetJob>(QString("%1::Search").arg(debugName()), APPLICATION->network());

    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(search_url), response));

    QObject::connect(netJob.get(), &TaskV2::finished, [this, netJob, response, callbacks](TaskV2* t) {
        switch (t->state()) {
            case TaskV2::Succeeded: {
                QJsonParseError parse_error{};
                QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
                if (parse_error.error != QJsonParseError::NoError) {
                    qWarning() << "Error while parsing JSON response from " << debugName() << " at " << parse_error.offset
                               << " reason: " << parse_error.errorString();
                    qWarning() << *response;

                    callbacks.on_fail(parse_error.errorString(), -1);

                    return;
                }

                callbacks.on_succeed(doc);
                break;
            }
            case TaskV2::AbortedByUser: {
                callbacks.on_abort();
                break;
            }
            case TaskV2::Inactive:
                [[fallthrough]];
            case TaskV2::Running:
                [[fallthrough]];
            case TaskV2::Paused:
                [[fallthrough]];
            case TaskV2::Finished:
                [[fallthrough]];
            case TaskV2::Failed: {
                int network_error_code = -1;
                if (auto* failed_action = netJob->getFailedActions().at(0); failed_action)
                    network_error_code = failed_action->replyStatusCode();

                callbacks.on_fail(t->failReason(), network_error_code);
                break;
            }
        }
    });

    return netJob;
}

TaskV2::Ptr NetworkResourceAPI::getProjectInfo(ProjectInfoArgs&& args, ProjectInfoCallbacks&& callbacks) const
{
    auto response = std::make_shared<QByteArray>();
    auto job = getProject(args.pack.addonId.toString(), response);

    QObject::connect(job.get(), &TaskV2::finished, [response, callbacks, args](TaskV2* t) {
        switch (t->state()) {
            case TaskV2::Succeeded: {
                QJsonParseError parse_error{};
                QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
                if (parse_error.error != QJsonParseError::NoError) {
                    qWarning() << "Error while parsing JSON response for mod info at " << parse_error.offset
                               << " reason: " << parse_error.errorString();
                    qWarning() << *response;
                    return;
                }

                callbacks.on_succeed(doc, args.pack);
                break;
            }
            case TaskV2::AbortedByUser: {
                callbacks.on_abort();
                break;
            }
            case TaskV2::Inactive:
                [[fallthrough]];
            case TaskV2::Running:
                [[fallthrough]];
            case TaskV2::Paused:
                [[fallthrough]];
            case TaskV2::Finished:
                [[fallthrough]];
            case TaskV2::Failed: {
                callbacks.on_fail(t->failReason());
                break;
            }
        }
    });
    return job;
}

TaskV2::Ptr NetworkResourceAPI::getProjectVersions(VersionSearchArgs&& args, VersionSearchCallbacks&& callbacks) const
{
    auto versions_url_optional = getVersionsURL(args);
    if (!versions_url_optional.has_value())
        return nullptr;

    auto versions_url = versions_url_optional.value();

    auto netJob = makeShared<NetJob>(QString("%1::Versions").arg(args.pack.name), APPLICATION->network());
    auto response = std::make_shared<QByteArray>();

    netJob->addNetAction(Net::ApiDownload::makeByteArray(versions_url, response));

    QObject::connect(netJob.get(), &TaskV2::finished, [response, callbacks, args, netJob](TaskV2* t) {
        if (t->wasSuccessful()) {
            QJsonParseError parse_error{};
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if (parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response for getting versions at " << parse_error.offset
                           << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }

            callbacks.on_succeed(doc, args.pack);
        } else {
            int network_error_code = -1;
            if (auto* failed_action = netJob->getFailedActions().at(0); failed_action)
                network_error_code = failed_action->replyStatusCode();

            callbacks.on_fail(t->failReason(), network_error_code);
        }
    });

    return netJob;
}

TaskV2::Ptr NetworkResourceAPI::getProject(QString addonId, std::shared_ptr<QByteArray> response) const
{
    auto project_url_optional = getInfoURL(addonId);
    if (!project_url_optional.has_value())
        return nullptr;

    auto project_url = project_url_optional.value();

    auto netJob = makeShared<NetJob>(QString("%1::GetProject").arg(addonId), APPLICATION->network());

    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(project_url), response));

    return netJob;
}

TaskV2::Ptr NetworkResourceAPI::getDependencyVersion(DependencySearchArgs&& args, DependencySearchCallbacks&& callbacks) const
{
    auto versions_url_optional = getDependencyURL(args);
    if (!versions_url_optional.has_value())
        return nullptr;

    auto versions_url = versions_url_optional.value();

    auto netJob = makeShared<NetJob>(QString("%1::Dependency").arg(args.dependency.addonId.toString()), APPLICATION->network());
    auto response = std::make_shared<QByteArray>();

    netJob->addNetAction(Net::ApiDownload::makeByteArray(versions_url, response));

    QObject::connect(netJob.get(), &TaskV2::finished, [response, callbacks, args, netJob](TaskV2* t) {
        if (t->wasSuccessful()) {
            QJsonParseError parse_error{};
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if (parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response for getting versions at " << parse_error.offset
                           << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }

            callbacks.on_succeed(doc, args.dependency);
        } else {
            int network_error_code = -1;
            if (auto* failed_action = netJob->getFailedActions().at(0); failed_action)
                network_error_code = failed_action->replyStatusCode();

            callbacks.on_fail(t->failReason(), network_error_code);
        }
    });
    return netJob;
}
