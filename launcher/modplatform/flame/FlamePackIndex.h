#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVector>
#include "modplatform/ModIndex.h"

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
};

struct ModpackExtra {
    QString websiteUrl;
    QString wikiUrl;
    QString issuesUrl;
    QString sourceUrl;
};

struct IndexedPack {
    int addonId;
    QString name;
    QString description;
    QList<ModpackAuthor> authors;
    QString logoName;
    QString logoUrl;

    bool versionsLoaded = false;
    QVector<IndexedVersion> versions;

    bool extraInfoLoaded = false;
    ModpackExtra extra;
};

void loadIndexedPack(IndexedPack& m, QJsonObject& obj);
void loadIndexedInfo(IndexedPack&, QJsonObject&);
void loadIndexedPackVersions(IndexedPack& m, QJsonArray& arr);
}  // namespace Flame

Q_DECLARE_METATYPE(Flame::IndexedPack)
