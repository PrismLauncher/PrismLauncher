#pragma once

#include "modplatform/ModAPI.h"
#include "ui/pages/modplatform/ModModel.h"

#include "Application.h"
#include "net/NetJob.h"

#include <QDebug>

class ModrinthAPI : public ModAPI {
   public:
    inline void searchMods(CallerType* caller, SearchArgs&& args) const override
    {
        auto netJob = new NetJob(QString("Modrinth::Search"), APPLICATION->network());
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

    inline void getVersions(CallerType* caller, const QString& addonId, const QString& debugName = "Modrinth") const override
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

    inline QString getAuthorURL(const QString& name) const { return "https://modrinth.com/user/" + name; };

   private:
    inline QString getModSearchURL(SearchArgs& args) const
    {
        if (!validateModLoader(args.mod_loader)) {
            qWarning() << "Modrinth only have Forge and Fabric-compatible mods!";
            return "";
        }

        return QString(
                   "https://api.modrinth.com/v2/search?"
                   "offset=%1&"
                   "limit=25&"
                   "query=%2&"
                   "index=%3&"
                   "facets=[[\"categories:%4\"],[\"versions:%5\"],[\"project_type:mod\"]]")
            .arg(args.offset)
            .arg(args.search)
            .arg(args.sorting)
            .arg(getModLoaderString(args.mod_loader))
            .arg(args.version);
    };

    inline QString getVersionsURL(const QString& addonId) const
    {
        return QString("https://api.modrinth.com/v2/project/%1/version").arg(addonId);
    };

    inline bool validateModLoader(ModLoaderType modLoader) const { return modLoader == Any || modLoader == Forge || modLoader == Fabric; }

    inline QString getModLoaderString(ModLoaderType modLoader) const
    {
        switch (modLoader) {
            case Any:
                return "fabric, forge";
            case Forge:
                return "forge";
            case Fabric:
                return "fabric";
            default:
                return "";
        }
    }
};
