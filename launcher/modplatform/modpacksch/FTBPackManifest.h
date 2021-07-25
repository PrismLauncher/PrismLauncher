#pragma once

#include <QString>
#include <QVector>
#include <QUrl>
#include <QJsonObject>
#include <QMetaType>

namespace ModpacksCH
{

struct Specs
{
    int id;
    int minimum;
    int recommended;
};

struct Tag
{
    int id;
    QString name;
};

struct Art
{
    int id;
    QString url;
    QString type;
    int width;
    int height;
    bool compressed;
    QString sha1;
    int size;
    int64_t updated;
};

struct Author
{
    int id;
    QString name;
    QString type;
    QString website;
    int64_t updated;
};

struct VersionInfo
{
    int id;
    QString name;
    QString type;
    int64_t updated;
    Specs specs;
};

struct Modpack
{
    int id;
    QString name;
    QString synopsis;
    QString description;
    QString type;
    bool featured;
    int installs;
    int plays;
    int64_t updated;
    int64_t refreshed;
    QVector<Art> art;
    QVector<Author> authors;
    QVector<VersionInfo> versions;
    QVector<Tag> tags;
};

struct VersionTarget
{
    int id;
    QString type;
    QString name;
    QString version;
    int64_t updated;
};

struct VersionFile
{
    int id;
    QString type;
    QString path;
    QString name;
    QString version;
    QString url;
    QString sha1;
    int size;
    bool clientOnly;
    bool serverOnly;
    bool optional;
    int64_t updated;
};

struct Version
{
    int id;
    int parent;
    QString name;
    QString type;
    int installs;
    int plays;
    int64_t updated;
    int64_t refreshed;
    Specs specs;
    QVector<VersionTarget> targets;
    QVector<VersionFile> files;
};

struct VersionChangelog
{
    QString content;
    int64_t updated;
};

void loadModpack(Modpack & m, QJsonObject & obj);

void loadVersion(Version & m, QJsonObject & obj);
}

Q_DECLARE_METATYPE(ModpacksCH::Modpack)
