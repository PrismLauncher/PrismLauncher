//
// Created by timoreo on 16/01/2022.
//

#pragma once
#include <QList>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <QNetworkAccessManager>
#include <QObjectPtr.h>
#include "net/NetJob.h"
#include "BaseInstance.h"

namespace FlameMod {
    struct ModpackAuthor {
        QString name;
        QString url;
    };

    struct IndexedVersion {
        int addonId;
        int fileId;
        QString version;
        QVector<QString> mcVersion;
        QString downloadUrl;
        QString date;
        QString fileName;
    };

    struct IndexedPack
    {
        int addonId;
        QString name;
        QString description;
        QList<ModpackAuthor> authors;
        QString logoName;
        QString logoUrl;
        QString websiteUrl;

        bool versionsLoaded = false;
        QVector<IndexedVersion> versions;
    };

    void loadIndexedPack(IndexedPack & m, QJsonObject & obj);
    void loadIndexedPackVersions(IndexedPack &pack, QJsonArray &arr, const shared_qobject_ptr<QNetworkAccessManager> &network, BaseInstance *inst);

}

Q_DECLARE_METATYPE(FlameMod::IndexedPack)
