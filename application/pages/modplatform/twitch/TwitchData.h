#pragma once

#include <QString>
#include <QList>

namespace Twitch {

struct ModpackAuthor {
    QString name;
    QString url;
};

struct ModpackFile {
    int addonId;
    int fileId;
    QString version;
    QString mcVersion;
    QString downloadUrl;
};

struct Modpack
{
    bool broken = true;
    int addonId = 0;

    QString name;
    QString description;
    QList<ModpackAuthor> authors;
    QString mcVersion;
    QString logoName;
    QString logoUrl;
    QString websiteUrl;

    ModpackFile latestFile;
};
}

Q_DECLARE_METATYPE(Twitch::Modpack)
