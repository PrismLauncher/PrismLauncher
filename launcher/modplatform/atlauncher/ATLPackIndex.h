#pragma once

#include "ATLPackManifest.h"

#include <QString>
#include <QVector>
#include <QMetaType>

namespace ATLauncher
{

struct IndexedVersion
{
    QString version;
    QString minecraft;
};

struct IndexedPack
{
    int id;
    int position;
    QString name;
    PackType type;
    QVector<IndexedVersion> versions;
    bool system;
    QString description;

    QString safeName;
};

void loadIndexedPack(IndexedPack & m, QJsonObject & obj);
}

Q_DECLARE_METATYPE(ATLauncher::IndexedPack)
