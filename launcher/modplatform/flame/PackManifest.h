#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QUrl>
#include <QJsonObject>

namespace Flame
{
struct File
{
    // NOTE: throws JSONValidationError
    bool parseFromObject(const QJsonObject& object);

    int projectId = 0;
    int fileId = 0;
    // NOTE: the opposite to 'optional'. This is at the time of writing unused.
    bool required = true;
    QString hash;
    // NOTE: only set on blocked files ! Empty otherwise.
    QString websiteUrl;

    // our
    bool resolved = false;
    QString fileName;
    QUrl url;
    QString targetFolder = QStringLiteral("mods");
    enum class Type
    {
        Unknown,
        Folder,
        Ctoc,
        SingleFile,
        Cmod2,
        Modpack,
        Mod
    } type = Type::Mod;
};

struct Modloader
{
    QString id;
    bool primary = false;
};

struct Minecraft
{
    QString version;
    QString libraries;
    QVector<Flame::Modloader> modLoaders;
};

struct Manifest
{
    QString manifestType;
    int manifestVersion = 0;
    Flame::Minecraft minecraft;
    QString name;
    QString version;
    QString author;
    //File id -> File
    QMap<int,Flame::File> files;
    QString overrides;
};

void loadManifest(Flame::Manifest & m, const QString &filepath);
}
