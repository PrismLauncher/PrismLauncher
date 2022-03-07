#include "NetworkModAPI.h"

#include "ui/pages/modplatform/ModModel.h"

#include "Application.h"
#include "net/NetJob.h"

void NetworkModAPI::searchMods(CallerType* caller, SearchArgs&& args) const
{
    auto netJob = new NetJob(QString("Modrinth::Search"), APPLICATION->network());
    auto searchUrl = getModSearchURL(args);

    auto response = new QByteArray();
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), response));

    QObject::connect(netJob, &NetJob::started, caller, [caller, netJob] { caller->setActiveJob(netJob); });
    QObject::connect(netJob, &NetJob::failed, caller, &CallerType::searchRequestFailed);
    QObject::connect(netJob, &NetJob::succeeded, caller, [caller, response] {
        QJsonParseError parse_error;
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

void NetworkModAPI::getVersions(CallerType* caller, const QString& addonId) const
{
    auto netJob = new NetJob(QString("%1::ModVersions(%2)").arg(caller->debugName()).arg(addonId), APPLICATION->network());
    auto response = new QByteArray();

    netJob->addNetAction(Net::Download::makeByteArray(getVersionsURL(addonId), response));

    QObject::connect(netJob, &NetJob::succeeded, caller, [response, caller, addonId] {
        QJsonParseError parse_error;
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << caller->debugName() << " at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        caller->versionRequestSucceeded(doc, addonId);
    });

    QObject::connect(netJob, &NetJob::finished, caller, [response, netJob] {
        netJob->deleteLater();
        delete response;
    });

    netJob->start();
}
