#pragma once

#include "modplatform/ModAPI.h"
#include "ui/pages/modplatform/ModModel.h"

#include "Application.h"
#include "net/NetJob.h"

class FlameAPI : public ModAPI {
   public:
    inline void searchMods(CallerType* caller, SearchArgs&& args) const override
    {
        auto netJob = new NetJob(QString("Flame::Search"), APPLICATION->network());
        auto searchUrl = getModSearchURL(args);

        auto response = new QByteArray();
        netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), response));

        QObject::connect(netJob, &NetJob::started, caller, [caller, netJob]{ caller->setActiveJob(netJob); });
        QObject::connect(netJob, &NetJob::failed, caller, &CallerType::searchRequestFailed);
        QObject::connect(netJob, &NetJob::succeeded, caller, [caller, response] {
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if (parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from Modrinth at " << parse_error.offset
                           << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }

            caller->searchRequestFinished(doc);
        });

        netJob->start();
    };

    inline void getVersions(CallerType* caller, const QString& addonId, const QString& debugName = "Flame") const override
    {
        auto netJob = new NetJob(QString("%1::ModVersions(%2)").arg(debugName).arg(addonId), APPLICATION->network());
        auto response = new QByteArray();

        netJob->addNetAction(Net::Download::makeByteArray(getVersionsURL(addonId), response));

        QObject::connect(netJob, &NetJob::succeeded, caller, [response, debugName, caller, addonId] {
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if (parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from " << debugName << " at " << parse_error.offset
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
    };

   private:
    inline QString getModSearchURL(SearchArgs& args) const
    {
        return QString(
                   "https://addons-ecs.forgesvc.net/api/v2/addon/search?"
                   "gameId=432&"
                   "categoryId=0&"
                   "sectionId=6&"

                   "index=%1&"
                   "pageSize=25&"
                   "searchFilter=%2&"
                   "sort=%3&"
                   "modLoaderType=%4&"
                   "gameVersion=%5")
            .arg(args.offset)
            .arg(args.search)
            .arg(args.sorting)
            .arg(args.mod_loader)
            .arg(args.version);
    };

    inline QString getVersionsURL(const QString& addonId) const
    {
        return QString("https://addons-ecs.forgesvc.net/api/v2/addon/%1/files").arg(addonId);
    };

    inline QString getAuthorURL(const QString& name) const { return ""; };
};
