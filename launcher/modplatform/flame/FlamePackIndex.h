#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVector>

namespace Flame {

struct ModpackAuthor {
    QString name;
    QString url;
};

struct IndexedVersion {
    int addonId;
    int fileId;
    QString version;
    QString mcVersion;
    QString downloadUrl;
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
void loadIndexedPackVersions(IndexedPack & m, QJsonArray & arr);
}

Q_DECLARE_METATYPE(Flame::IndexedPack)
