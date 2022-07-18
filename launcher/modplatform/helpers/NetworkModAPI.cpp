#include "NetworkModAPI.h"

#include "ui/pages/modplatform/ModModel.h"

#include "Application.h"
#include "net/NetJob.h"

void NetworkModAPI::searchMods(CallerType* caller, SearchArgs&& args) const
{
    auto netJob = new NetJob(QString("%1::Search").arg(caller->debugName()), APPLICATION->network());
    auto searchUrl = getModSearchURL(args);

    auto response = new QByteArray();
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), response));

    QObject::connect(netJob, &NetJob::started, caller, [caller, netJob] { caller->setActiveJob(netJob); });
    QObject::connect(netJob, &NetJob::failed, caller, &CallerType::searchRequestFailed);
    QObject::connect(netJob, &NetJob::succeeded, caller, [caller, response] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << caller->debugName() << " at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        caller->searchRequestFinished(doc);
    });

    netJob->start();
}

void NetworkModAPI::getModInfo(CallerType* caller, ModPlatform::IndexedPack& pack)
{
    auto response = new QByteArray();
    auto job = getProject(pack.addonId.toString(), response);

    QObject::connect(job, &NetJob::succeeded, caller, [caller, &pack, response] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << caller->debugName() << " at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        caller->infoRequestFinished(doc, pack);
    });

    job->start();
}

void NetworkModAPI::getVersions(CallerType* caller, VersionSearchArgs&& args) const
{
    auto netJob = new NetJob(QString("%1::ModVersions(%2)").arg(caller->debugName()).arg(args.addonId), APPLICATION->network());
    auto response = new QByteArray();

    netJob->addNetAction(Net::Download::makeByteArray(getVersionsURL(args), response));

    QObject::connect(netJob, &NetJob::succeeded, caller, [response, caller, args] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << caller->debugName() << " at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        caller->versionRequestSucceeded(doc, args.addonId);
    });

    QObject::connect(netJob, &NetJob::finished, caller, [response, netJob] {
        netJob->deleteLater();
        delete response;
    });

    netJob->start();
}

auto NetworkModAPI::getProject(QString addonId, QByteArray* response) const -> NetJob*
{
    auto netJob = new NetJob(QString("%1::GetProject").arg(addonId), APPLICATION->network());
    auto searchUrl = getModInfoURL(addonId);

    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), response));

    QObject::connect(netJob, &NetJob::finished, [response, netJob] {
        netJob->deleteLater();
        delete response;
    });

    return netJob;
}
