#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <QVector>

namespace ModPlatform {

struct ModpackAuthor {
    QString name;
    QString url;
};

struct DonationData {
    QString id;
    QString platform;
    QString url;
};

struct IndexedVersion {
    QVariant addonId;
    QVariant fileId;
    QString version;
    QVector<QString> mcVersion;
    QString downloadUrl;
    QString date;
    QString fileName;
    QVector<QString> loaders = {};
};

struct ExtraPackData {
    QList<DonationData> donate;

    QString issuesUrl;
    QString sourceUrl;
    QString wikiUrl;
    QString discordUrl;
};

struct IndexedPack {
    QVariant addonId;
    QString name;
    QString description;
    QList<ModpackAuthor> authors;
    QString logoName;
    QString logoUrl;
    QString websiteUrl;

    bool versionsLoaded = false;
    QVector<IndexedVersion> versions;

    // Don't load by default, since some modplatform don't have that info
    bool extraDataLoaded = true;
    ExtraPackData extraData;
};

}  // namespace ModPlatform

Q_DECLARE_METATYPE(ModPlatform::IndexedPack)
