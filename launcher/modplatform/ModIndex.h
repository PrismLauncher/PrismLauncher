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
};

}  // namespace ModPlatform

Q_DECLARE_METATYPE(ModPlatform::IndexedPack)
